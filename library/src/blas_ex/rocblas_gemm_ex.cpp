/* ************************************************************************
 * Copyright (C) 2016-2023 Advanced Micro Devices, Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell cop-
 * ies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IM-
 * PLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNE-
 * CTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * ************************************************************************ */

#include "rocblas_gemm_ex.hpp"
#include "handle.hpp"
#include "logging.hpp"
#include "rocblas.h"
#include "rocblas_gemm_ex_get_solutions.hpp"
#include "utility.hpp"

namespace
{
    rocblas_status rocblas_gemm_ex_impl(rocblas_handle    handle,
                                        rocblas_operation trans_a,
                                        rocblas_operation trans_b,
                                        rocblas_int       m,
                                        rocblas_int       n,
                                        rocblas_int       k,
                                        const void*       alpha,
                                        const void*       a,
                                        rocblas_datatype  a_type,
                                        rocblas_int       lda,
                                        const void*       b,
                                        rocblas_datatype  b_type,
                                        rocblas_int       ldb,
                                        const void*       beta,
                                        const void*       c,
                                        rocblas_datatype  c_type,
                                        rocblas_int       ldc,
                                        void*             d,
                                        rocblas_datatype  d_type,
                                        rocblas_int       ldd,
                                        rocblas_datatype  compute_type,
                                        rocblas_gemm_algo algo,
                                        int32_t           solution_index,
                                        uint32_t          flags)
    {

        if(!handle)
            return rocblas_status_invalid_handle;

        const bool HPA = compute_type == rocblas_datatype_f32_r
                         && (a_type == rocblas_datatype_f16_r || a_type == rocblas_datatype_bf16_r);

        if(!HPA)
            RETURN_ZERO_DEVICE_MEMORY_SIZE_IF_QUERIED(handle);

        // Copy alpha and beta to host if on device
        rocblas_union_t alpha_h, beta_h;
        RETURN_IF_ROCBLAS_ERROR(rocblas_copy_alpha_beta_to_host_if_on_device(
            handle, alpha, beta, alpha_h, beta_h, k, compute_type));
        auto saved_pointer_mode = handle->push_pointer_mode(rocblas_pointer_mode_host);

        // If this is a solution fitness query (internal testing), bypass logging and error checks
        if(handle->get_solution_fitness_query())
            goto solution_fitness_query;

        if(!handle->is_device_memory_size_query())
        {
            // Perform logging
            auto layer_mode = handle->layer_mode;
            if(layer_mode
               & (rocblas_layer_mode_log_trace | rocblas_layer_mode_log_bench
                  | rocblas_layer_mode_log_profile))
            {
                char trans_a_letter, trans_b_letter;
                if(layer_mode & (rocblas_layer_mode_log_bench | rocblas_layer_mode_log_profile))
                {
                    trans_a_letter = rocblas_transpose_letter(trans_a);
                    trans_b_letter = rocblas_transpose_letter(trans_b);
                }
                auto a_type_string       = rocblas_datatype_string(a_type);
                auto b_type_string       = rocblas_datatype_string(b_type);
                auto c_type_string       = rocblas_datatype_string(c_type);
                auto d_type_string       = rocblas_datatype_string(d_type);
                auto compute_type_string = rocblas_datatype_string(compute_type);

                if(layer_mode & rocblas_layer_mode_log_trace)
                {
                    rocblas_internal_ostream alphass, betass;
                    if(log_trace_alpha_beta_ex(compute_type, alpha, beta, alphass, betass)
                       == rocblas_status_success)
                    {
                        log_trace(handle,
                                  "rocblas_gemm_ex",
                                  trans_a,
                                  trans_b,
                                  m,
                                  n,
                                  k,
                                  alphass.str(),
                                  a,
                                  a_type_string,
                                  lda,
                                  b,
                                  b_type_string,
                                  ldb,
                                  betass.str(),
                                  c,
                                  c_type_string,
                                  ldc,
                                  d,
                                  d_type_string,
                                  ldd,
                                  compute_type_string,
                                  algo,
                                  solution_index,
                                  rocblas_gemm_flags(flags));
                    }
                }

                if(layer_mode & rocblas_layer_mode_log_bench)
                {
                    std::string alphas, betas;
                    if(log_bench_alpha_beta_ex(compute_type, alpha, beta, alphas, betas)
                       == rocblas_status_success)
                    {

                        log_bench(handle,
                                  "./rocblas-bench -f gemm_ex",
                                  "--transposeA",
                                  trans_a_letter,
                                  "--transposeB",
                                  trans_b_letter,
                                  "-m",
                                  m,
                                  "-n",
                                  n,
                                  "-k",
                                  k,
                                  alphas,
                                  "--a_type",
                                  a_type_string,
                                  "--lda",
                                  lda,
                                  "--b_type",
                                  b_type_string,
                                  "--ldb",
                                  ldb,
                                  betas,
                                  "--c_type",
                                  c_type_string,
                                  "--ldc",
                                  ldc,
                                  "--d_type",
                                  d_type_string,
                                  "--ldd",
                                  ldd,
                                  "--compute_type",
                                  compute_type_string,
                                  "--algo",
                                  algo,
                                  "--solution_index",
                                  solution_index,
                                  "--flags",
                                  flags);
                    }
                }

                if(layer_mode & rocblas_layer_mode_log_profile)
                {
                    log_profile(handle,
                                "rocblas_gemm_ex",
                                "a_type",
                                a_type_string,
                                "b_type",
                                b_type_string,
                                "c_type",
                                c_type_string,
                                "d_type",
                                d_type_string,
                                "compute_type",
                                compute_type_string,
                                "transA",
                                trans_a_letter,
                                "transB",
                                trans_b_letter,
                                "M",
                                m,
                                "N",
                                n,
                                "K",
                                k,
                                "alpha",
                                value_category(alpha, compute_type),
                                "lda",
                                lda,
                                "ldb",
                                ldb,
                                "beta",
                                value_category(beta, compute_type),
                                "ldc",
                                ldc,
                                "ldd",
                                ldd,
                                "algo",
                                algo,
                                "solution_index",
                                solution_index,
                                "flags",
                                rocblas_gemm_flags(flags));
                }
            }
        }

        {
            auto validArgs = rocblas_validateArgs(handle,
                                                  trans_a,
                                                  trans_b,
                                                  m,
                                                  n,
                                                  k,
                                                  alpha,
                                                  a,
                                                  lda,
                                                  b,
                                                  ldb,
                                                  beta,
                                                  c,
                                                  c_type,
                                                  ldc,
                                                  d,
                                                  d_type,
                                                  ldd,
                                                  compute_type);

            if(validArgs != rocblas_status_continue)
            {
                if(validArgs == rocblas_status_success)
                    RETURN_ZERO_DEVICE_MEMORY_SIZE_IF_QUERIED(handle);
                return validArgs;
            }
        }

    solution_fitness_query:
        rocblas_int batch_count = 1;

        // TODO: These strides could be 0 ( {} ) instead of 1 ( {1} ) once Tensile is fixed
        rocblas_stride stride_a{1}, stride_b{1}, stride_c{1}, stride_d{1};

        return rocblas_gemm_ex_template<false>(handle,
                                               trans_a,
                                               trans_b,
                                               m,
                                               n,
                                               k,
                                               alpha,
                                               a,
                                               a_type,
                                               0,
                                               lda,
                                               stride_a,
                                               b,
                                               b_type,
                                               0,
                                               ldb,
                                               stride_b,
                                               beta,
                                               c,
                                               c_type,
                                               0,
                                               ldc,
                                               stride_c,
                                               d,
                                               d_type,
                                               0,
                                               ldd,
                                               stride_d,
                                               batch_count,
                                               compute_type,
                                               algo,
                                               solution_index,
                                               flags);
    }
}
// namespace

extern "C" rocblas_status rocblas_gemm_ex(rocblas_handle    handle,
                                          rocblas_operation trans_a,
                                          rocblas_operation trans_b,
                                          rocblas_int       m,
                                          rocblas_int       n,
                                          rocblas_int       k,
                                          const void*       alpha,
                                          const void*       a,
                                          rocblas_datatype  a_type,
                                          rocblas_int       lda,
                                          const void*       b,
                                          rocblas_datatype  b_type,
                                          rocblas_int       ldb,
                                          const void*       beta,
                                          const void*       c,
                                          rocblas_datatype  c_type,
                                          rocblas_int       ldc,
                                          void*             d,
                                          rocblas_datatype  d_type,
                                          rocblas_int       ldd,
                                          rocblas_datatype  compute_type,
                                          rocblas_gemm_algo algo,
                                          int32_t           solution_index,
                                          uint32_t          flags)
try
{
    return rocblas_gemm_ex_impl(handle,
                                trans_a,
                                trans_b,
                                m,
                                n,
                                k,
                                alpha,
                                a,
                                a_type,
                                lda,
                                b,
                                b_type,
                                ldb,
                                beta,
                                c,
                                c_type,
                                ldc,
                                d,
                                d_type,
                                ldd,
                                compute_type,
                                algo,
                                solution_index,
                                flags);
}
catch(...)
{
    return exception_to_rocblas_status();
}

extern "C" rocblas_status rocblas_gemm_ex_get_solutions(rocblas_handle    handle,
                                                        rocblas_operation trans_a,
                                                        rocblas_operation trans_b,
                                                        rocblas_int       m,
                                                        rocblas_int       n,
                                                        rocblas_int       k,
                                                        const void*       alpha,
                                                        const void*       a,
                                                        rocblas_datatype  a_type,
                                                        rocblas_int       lda,
                                                        const void*       b,
                                                        rocblas_datatype  b_type,
                                                        rocblas_int       ldb,
                                                        const void*       beta,
                                                        const void*       c,
                                                        rocblas_datatype  c_type,
                                                        rocblas_int       ldc,
                                                        void*             d,
                                                        rocblas_datatype  d_type,
                                                        rocblas_int       ldd,
                                                        rocblas_datatype  compute_type,
                                                        rocblas_gemm_algo algo,
                                                        uint32_t          flags,
                                                        rocblas_int*      list_array,
                                                        rocblas_int*      list_size)
{
    try
    {
        if(!handle)
            return rocblas_status_invalid_handle;

        const bool HPA = compute_type == rocblas_datatype_f32_r
                         && (a_type == rocblas_datatype_f16_r || a_type == rocblas_datatype_bf16_r);

        if(!HPA)
            RETURN_ZERO_DEVICE_MEMORY_SIZE_IF_QUERIED(handle);

        auto validArgs = rocblas_validateArgs(handle,
                                              trans_a,
                                              trans_b,
                                              m,
                                              n,
                                              k,
                                              alpha,
                                              a,
                                              lda,
                                              b,
                                              ldb,
                                              beta,
                                              c,
                                              c_type,
                                              ldc,
                                              d,
                                              d_type,
                                              ldd,
                                              compute_type);

        if(validArgs != rocblas_status_continue)
        {
            if(validArgs == rocblas_status_success)
                RETURN_ZERO_DEVICE_MEMORY_SIZE_IF_QUERIED(handle);
            return validArgs;
        }

        rocblas_int batch_count = 1;

        // TODO: These strides could be 0 ( {} ) instead of 1 ( {1} ) once Tensile is fixed
        rocblas_stride stride_a{1}, stride_b{1}, stride_c{1}, stride_d{1};

        return rocblas_gemm_ex_get_solutions_template<false>(handle,
                                                             trans_a,
                                                             trans_b,
                                                             m,
                                                             n,
                                                             k,
                                                             alpha,
                                                             a,
                                                             a_type,
                                                             0,
                                                             lda,
                                                             stride_a,
                                                             b,
                                                             b_type,
                                                             0,
                                                             ldb,
                                                             stride_b,
                                                             beta,
                                                             c,
                                                             c_type,
                                                             0,
                                                             ldc,
                                                             stride_c,
                                                             d,
                                                             d_type,
                                                             0,
                                                             ldd,
                                                             stride_d,
                                                             batch_count,
                                                             compute_type,
                                                             flags,
                                                             CAN_SOLVE,
                                                             list_array,
                                                             list_size);
    }
    catch(...)
    {
        return exception_to_rocblas_status();
    }
}

ROCBLAS_EXPORT rocblas_status rocblas_gemm_ex_get_solutions_by_type(rocblas_handle   handle,
                                                                    rocblas_datatype input_type,
                                                                    rocblas_datatype output_type,
                                                                    rocblas_datatype compute_type,
                                                                    uint32_t         flags,
                                                                    rocblas_int*     list_array,
                                                                    rocblas_int*     list_size)
{
    // Create dummy GEMM problem to take advantage of problem templating
    // Most parameters are ignored, just needs to be valid for all types
    float          alpha = 0.0f;
    float          beta  = 0.0f;
    rocblas_stride stride{1};
    return rocblas_gemm_ex_get_solutions_template<false>(handle,
                                                         rocblas_operation_none,
                                                         rocblas_operation_none,
                                                         4,
                                                         4,
                                                         4,
                                                         &alpha,
                                                         NULL,
                                                         input_type,
                                                         0,
                                                         4,
                                                         stride,
                                                         NULL,
                                                         input_type,
                                                         0,
                                                         4,
                                                         stride,
                                                         &beta,
                                                         NULL,
                                                         output_type,
                                                         0,
                                                         4,
                                                         stride,
                                                         NULL,
                                                         output_type,
                                                         0,
                                                         4,
                                                         stride,
                                                         1,
                                                         compute_type,
                                                         flags,
                                                         MATCHES_TYPE,
                                                         list_array,
                                                         list_size);
}
