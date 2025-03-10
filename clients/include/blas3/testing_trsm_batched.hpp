/* ************************************************************************
 * Copyright (C) 2018-2023 Advanced Micro Devices, Inc. All rights reserved.
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

#pragma once

#include "cblas_interface.hpp"
#include "flops.hpp"
#include "norm.hpp"
#include "rocblas.hpp"
#include "rocblas_datatype2string.hpp"
#include "rocblas_init.hpp"
#include "rocblas_math.hpp"
#include "rocblas_matrix.hpp"
#include "rocblas_random.hpp"
#include "rocblas_test.hpp"
#include "rocblas_vector.hpp"
#include "unit.hpp"
#include "utility.hpp"

#include "blas3/rocblas_trsm.hpp"

#define ERROR_EPS_MULTIPLIER 40
#define RESIDUAL_EPS_MULTIPLIER 40

template <typename T>
void testing_trsm_batched_bad_arg(const Arguments& arg)
{
    auto rocblas_trsm_batched_fn
        = arg.api == FORTRAN ? rocblas_trsm_batched<T, true> : rocblas_trsm_batched<T, false>;

    for(auto pointer_mode : {rocblas_pointer_mode_host, rocblas_pointer_mode_device})
    {
        rocblas_local_handle handle{arg};
        CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, pointer_mode));

        const rocblas_int M           = 100;
        const rocblas_int N           = 100;
        const rocblas_int lda         = 100;
        const rocblas_int ldb         = 100;
        const rocblas_int batch_count = 2;

        device_vector<T> alpha_d(1), zero_d(1);

        const T alpha_h(1), zero_h(0);

        const T* alpha = &alpha_h;
        const T* zero  = &zero_h;

        if(pointer_mode == rocblas_pointer_mode_device)
        {
            CHECK_HIP_ERROR(hipMemcpy(alpha_d, alpha, sizeof(*alpha), hipMemcpyHostToDevice));
            alpha = alpha_d;
            CHECK_HIP_ERROR(hipMemcpy(zero_d, zero, sizeof(*zero), hipMemcpyHostToDevice));
            zero = zero_d;
        }

        const rocblas_side      side   = rocblas_side_left;
        const rocblas_fill      uplo   = rocblas_fill_upper;
        const rocblas_operation transA = rocblas_operation_none;
        const rocblas_diagonal  diag   = rocblas_diagonal_non_unit;

        rocblas_int K = side == rocblas_side_left ? M : N;

        // Allocate device memory
        device_batch_matrix<T> dA(K, K, lda, batch_count);
        device_batch_matrix<T> dB(M, N, ldb, batch_count);

        // Check device memory allocation
        CHECK_DEVICE_ALLOCATION(dA.memcheck());
        CHECK_DEVICE_ALLOCATION(dB.memcheck());

        // check for invalid enum
        EXPECT_ROCBLAS_STATUS(rocblas_trsm_batched_fn(handle,
                                                      rocblas_side_both,
                                                      uplo,
                                                      transA,
                                                      diag,
                                                      M,
                                                      N,
                                                      alpha,
                                                      dA,
                                                      lda,
                                                      dB,
                                                      ldb,
                                                      batch_count),
                              rocblas_status_invalid_value);

        EXPECT_ROCBLAS_STATUS(rocblas_trsm_batched_fn(handle,
                                                      side,
                                                      (rocblas_fill)rocblas_side_both,
                                                      transA,
                                                      diag,
                                                      M,
                                                      N,
                                                      alpha,
                                                      dA,
                                                      lda,
                                                      dB,
                                                      ldb,
                                                      batch_count),
                              rocblas_status_invalid_value);

        EXPECT_ROCBLAS_STATUS(rocblas_trsm_batched_fn(handle,
                                                      side,
                                                      uplo,
                                                      (rocblas_operation)rocblas_side_both,
                                                      diag,
                                                      M,
                                                      N,
                                                      alpha,
                                                      dA,
                                                      lda,
                                                      dB,
                                                      ldb,
                                                      batch_count),
                              rocblas_status_invalid_value);

        EXPECT_ROCBLAS_STATUS(rocblas_trsm_batched_fn(handle,
                                                      side,
                                                      uplo,
                                                      transA,
                                                      (rocblas_diagonal)rocblas_side_both,
                                                      M,
                                                      N,
                                                      alpha,
                                                      dA,
                                                      lda,
                                                      dB,
                                                      ldb,
                                                      batch_count),
                              rocblas_status_invalid_value);

        // check for invalid size
        EXPECT_ROCBLAS_STATUS(
            rocblas_trsm_batched_fn(
                handle, side, uplo, transA, diag, -1, N, alpha, dA, lda, dB, ldb, batch_count),
            rocblas_status_invalid_size);

        EXPECT_ROCBLAS_STATUS(
            rocblas_trsm_batched_fn(
                handle, side, uplo, transA, diag, M, -1, alpha, dA, lda, dB, ldb, batch_count),
            rocblas_status_invalid_size);

        EXPECT_ROCBLAS_STATUS(
            rocblas_trsm_batched_fn(
                handle, side, uplo, transA, diag, M, N, alpha, dA, lda, dB, ldb, -1),
            rocblas_status_invalid_size);

        // check for invalid leading dimension
        EXPECT_ROCBLAS_STATUS(
            rocblas_trsm_batched_fn(
                handle, side, uplo, transA, diag, M, N, alpha, dA, lda, dB, M - 1, batch_count),
            rocblas_status_invalid_size);

        EXPECT_ROCBLAS_STATUS(rocblas_trsm_batched_fn(handle,
                                                      rocblas_side_left,
                                                      uplo,
                                                      transA,
                                                      diag,
                                                      M,
                                                      N,
                                                      alpha,
                                                      dA,
                                                      M - 1,
                                                      dB,
                                                      ldb,
                                                      batch_count),
                              rocblas_status_invalid_size);

        EXPECT_ROCBLAS_STATUS(rocblas_trsm_batched_fn(handle,
                                                      rocblas_side_right,
                                                      uplo,
                                                      transA,
                                                      diag,
                                                      M,
                                                      N,
                                                      alpha,
                                                      dA,
                                                      N - 1,
                                                      dB,
                                                      ldb,
                                                      batch_count),
                              rocblas_status_invalid_size);

        // check that nullpointer gives rocblas_status_invalid_handle or rocblas_status_invalid_pointer
        EXPECT_ROCBLAS_STATUS(
            rocblas_trsm_batched_fn(
                nullptr, side, uplo, transA, diag, M, N, alpha, dA, lda, dB, ldb, batch_count),
            rocblas_status_invalid_handle);

        EXPECT_ROCBLAS_STATUS(
            rocblas_trsm_batched_fn(
                handle, side, uplo, transA, diag, M, N, nullptr, dA, lda, dB, ldb, batch_count),
            rocblas_status_invalid_pointer);

        if(pointer_mode == rocblas_pointer_mode_host)
        {
            EXPECT_ROCBLAS_STATUS(rocblas_trsm_batched_fn(handle,
                                                          side,
                                                          uplo,
                                                          transA,
                                                          diag,
                                                          M,
                                                          N,
                                                          alpha,
                                                          nullptr,
                                                          lda,
                                                          dB,
                                                          ldb,
                                                          batch_count),
                                  rocblas_status_invalid_pointer);
        }

        EXPECT_ROCBLAS_STATUS(
            rocblas_trsm_batched_fn(
                handle, side, uplo, transA, diag, M, N, alpha, dA, lda, nullptr, ldb, batch_count),
            rocblas_status_invalid_pointer);

        // When batch_count==0, all pointers may be nullptr without error
        EXPECT_ROCBLAS_STATUS(
            rocblas_trsm_batched_fn(
                handle, side, uplo, transA, diag, M, N, nullptr, nullptr, lda, nullptr, ldb, 0),
            rocblas_status_success);

        // When M==0, all pointers may be nullptr without error
        EXPECT_ROCBLAS_STATUS(rocblas_trsm_batched_fn(handle,
                                                      side,
                                                      uplo,
                                                      transA,
                                                      diag,
                                                      0,
                                                      N,
                                                      nullptr,
                                                      nullptr,
                                                      lda,
                                                      nullptr,
                                                      ldb,
                                                      batch_count),
                              rocblas_status_success);

        // When N==0, all pointers may be nullptr without error
        EXPECT_ROCBLAS_STATUS(rocblas_trsm_batched_fn(handle,
                                                      side,
                                                      uplo,
                                                      transA,
                                                      diag,
                                                      M,
                                                      0,
                                                      nullptr,
                                                      nullptr,
                                                      lda,
                                                      nullptr,
                                                      ldb,
                                                      batch_count),
                              rocblas_status_success);

        // If alpha==0, then A can be nullptr without error
        EXPECT_ROCBLAS_STATUS(rocblas_trsm_batched_fn(handle,
                                                      side,
                                                      uplo,
                                                      transA,
                                                      diag,
                                                      M,
                                                      N,
                                                      zero,
                                                      nullptr,
                                                      lda,
                                                      dB.ptr_on_device(),
                                                      ldb,
                                                      batch_count),
                              rocblas_status_success);
    }
}

template <typename T>
void testing_trsm_batched(const Arguments& arg)
{
    auto rocblas_trsm_batched_fn
        = arg.api == FORTRAN ? rocblas_trsm_batched<T, true> : rocblas_trsm_batched<T, false>;

    rocblas_int M           = arg.M;
    rocblas_int N           = arg.N;
    rocblas_int lda         = arg.lda;
    rocblas_int ldb         = arg.ldb;
    rocblas_int batch_count = arg.batch_count;

    char char_side   = arg.side;
    char char_uplo   = arg.uplo;
    char char_transA = arg.transA;
    char char_diag   = arg.diag;
    T    alpha_h     = arg.alpha;

    rocblas_side      side   = char2rocblas_side(char_side);
    rocblas_fill      uplo   = char2rocblas_fill(char_uplo);
    rocblas_operation transA = char2rocblas_operation(char_transA);
    rocblas_diagonal  diag   = char2rocblas_diagonal(char_diag);

    // for internal interface testing: using ldc/ldd as offsets
    const bool     internal_api = arg.api == INTERNAL;
    rocblas_stride offsetA      = internal_api ? arg.ldc : 0;
    rocblas_stride offsetB      = internal_api ? arg.ldd : 0;

    rocblas_int K = side == rocblas_side_left ? M : N;

    rocblas_local_handle handle{arg};
    // check here to prevent undefined memory allocation error
    bool invalid_size = M < 0 || N < 0 || lda < K || ldb < M || batch_count < 0;
    if(invalid_size || batch_count == 0)
    {
        CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_host));
        EXPECT_ROCBLAS_STATUS(rocblas_trsm_batched_fn(handle,
                                                      side,
                                                      uplo,
                                                      transA,
                                                      diag,
                                                      M,
                                                      N,
                                                      nullptr,
                                                      nullptr,
                                                      lda,
                                                      nullptr,
                                                      ldb,
                                                      batch_count),
                              invalid_size ? rocblas_status_invalid_size : rocblas_status_success);
        return;
    }

    // Naming: `h` is in CPU (host) memory(eg hA), `d` is in GPU (device) memory (eg dA).
    // Allocate host memory
    host_batch_matrix<T> hA(K, K, lda, batch_count);
    host_batch_matrix<T> hB(M, N, ldb, batch_count);
    host_batch_matrix<T> hX(M, N, ldb, batch_count);
    host_batch_matrix<T> hXorB_1(M, N, ldb, batch_count);
    host_vector<T>       halpha(1);
    halpha[0] = alpha_h;

    // Check host memory allocation
    CHECK_HIP_ERROR(hA.memcheck());
    CHECK_HIP_ERROR(hB.memcheck());
    CHECK_HIP_ERROR(hX.memcheck());
    CHECK_HIP_ERROR(hXorB_1.memcheck());

    // Allocate device memory
    device_batch_matrix<T> dA(K, K, lda, batch_count, false, offsetA);
    device_batch_matrix<T> dXorB(M, N, ldb, batch_count, false, offsetB);
    device_vector<T>       alpha_d(1);

    // Check device memory allocation
    CHECK_DEVICE_ALLOCATION(dA.memcheck());
    CHECK_DEVICE_ALLOCATION(dXorB.memcheck());
    CHECK_DEVICE_ALLOCATION(alpha_d.memcheck());

    // Initialize data on host memory
    rocblas_init_matrix(hA,
                        arg,
                        rocblas_client_never_set_nan,
                        rocblas_client_diagonally_dominant_triangular_matrix,
                        true);
    rocblas_init_matrix(
        hX, arg, rocblas_client_never_set_nan, rocblas_client_general_matrix, false, true);

    //  make hA unit diagonal if diag == rocblas_diagonal_unit
    if(diag == rocblas_diagonal_unit)
    {
        make_unit_diagonal(uplo, hA);
    }

    hB.copy_from(hX);

    for(int b = 0; b < batch_count; b++)
    {
        // Calculate hB = hA*hX
        cblas_trmm<T>(side, uplo, transA, diag, M, N, 1.0 / alpha_h, hA[b], lda, hB[b], ldb);
    }

    hXorB_1.copy_from(hB);

    CHECK_HIP_ERROR(dA.transfer_from(hA));
    CHECK_HIP_ERROR(dXorB.transfer_from(hXorB_1));

    double gpu_time_used, cpu_time_used;
    gpu_time_used = cpu_time_used  = 0.0;
    double error_eps_multiplier    = ERROR_EPS_MULTIPLIER;
    double residual_eps_multiplier = RESIDUAL_EPS_MULTIPLIER;
    double eps                     = std::numeric_limits<real_t<T>>::epsilon();
    double err_host                = 0.0;
    double err_device              = 0.0;

    if(!ROCBLAS_REALLOC_ON_DEMAND)
    {
        // Compute size
        CHECK_ROCBLAS_ERROR(rocblas_start_device_memory_size_query(handle));
        CHECK_ALLOC_QUERY(rocblas_trsm_batched_fn(handle,
                                                  side,
                                                  uplo,
                                                  transA,
                                                  diag,
                                                  M,
                                                  N,
                                                  &alpha_h,
                                                  dA.ptr_on_device(),
                                                  lda,
                                                  dXorB.ptr_on_device(),
                                                  ldb,
                                                  batch_count));
        size_t size;
        CHECK_ROCBLAS_ERROR(rocblas_stop_device_memory_size_query(handle, &size));

        // Allocate memory
        CHECK_ROCBLAS_ERROR(rocblas_set_device_memory_size(handle, size));
    }

    if(arg.unit_check || arg.norm_check)
    {
        if(arg.pointer_mode_host)
        {
            // calculate dXorB <- A^(-1) B   rocblas_device_pointer_host
            CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_host));
            CHECK_HIP_ERROR(dXorB.transfer_from(hXorB_1));

            handle.pre_test(arg);
            if(!internal_api)
            {
                CHECK_ROCBLAS_ERROR(rocblas_trsm_batched_fn(handle,
                                                            side,
                                                            uplo,
                                                            transA,
                                                            diag,
                                                            M,
                                                            N,
                                                            &alpha_h,
                                                            dA.ptr_on_device(),
                                                            lda,
                                                            dXorB.ptr_on_device(),
                                                            ldb,
                                                            batch_count));
            }
            else
            {
                // internal function requires us to supply temporary memory ourselves
                constexpr bool BATCHED        = true;
                bool           optimal_mem    = true;
                rocblas_int    supp_invA_size = 0; // used for trsm_ex

                // first exported internal interface - calculate how much mem is needed
                size_t w_x_tmp_size, w_x_tmp_arr_size, w_invA_size, w_invA_arr_size,
                    w_x_tmp_size_backup;
                rocblas_status mem_status
                    = rocblas_internal_trsm_batched_workspace_size<T>(side,
                                                                      transA,
                                                                      M,
                                                                      N,
                                                                      batch_count,
                                                                      supp_invA_size,
                                                                      &w_x_tmp_size,
                                                                      &w_x_tmp_arr_size,
                                                                      &w_invA_size,
                                                                      &w_invA_arr_size,
                                                                      &w_x_tmp_size_backup);

                if(mem_status != rocblas_status_success && mem_status != rocblas_status_continue)
                    CHECK_ROCBLAS_ERROR(mem_status);

                // allocate memory ourselves
                device_vector<T>       w_mem_x_tmp(w_x_tmp_size / sizeof(T));
                device_batch_vector<T> w_mem_x_tmp_arr(1, 1, w_x_tmp_arr_size / sizeof(T*));
                device_vector<T>       w_mem_invA(w_invA_size / sizeof(T));
                device_batch_vector<T> w_mem_invA_arr(1, 1, w_invA_arr_size / sizeof(T*));

                rocblas_stride strideA = 0, strideB = 0;

                CHECK_ROCBLAS_ERROR(
                    rocblas_internal_trsm_batched_template(handle,
                                                           side,
                                                           uplo,
                                                           transA,
                                                           diag,
                                                           M,
                                                           N,
                                                           &alpha_h,
                                                           (const T* const*)dA.ptr_on_device(),
                                                           -offsetA,
                                                           lda,
                                                           strideA,
                                                           (T* const*)dXorB.ptr_on_device(),
                                                           -offsetB,
                                                           ldb,
                                                           strideB,
                                                           batch_count,
                                                           optimal_mem,
                                                           (void*)w_mem_x_tmp,
                                                           (void*)w_mem_x_tmp_arr.ptr_on_device(),
                                                           (void*)w_mem_invA,
                                                           (void*)w_mem_invA_arr.ptr_on_device()));
            }

            handle.post_test(arg);
            CHECK_HIP_ERROR(hXorB_1.transfer_from(dXorB));
        }

        if(arg.pointer_mode_device && !internal_api)
        {
            // calculate dXorB <- A^(-1) B   rocblas_device_pointer_device
            CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_device));
            CHECK_HIP_ERROR(alpha_d.transfer_from(halpha));
            CHECK_HIP_ERROR(dXorB.transfer_from(hB));

            CHECK_ROCBLAS_ERROR(rocblas_trsm_batched_fn(handle,
                                                        side,
                                                        uplo,
                                                        transA,
                                                        diag,
                                                        M,
                                                        N,
                                                        alpha_d,
                                                        dA.ptr_on_device(),
                                                        lda,
                                                        dXorB.ptr_on_device(),
                                                        ldb,
                                                        batch_count));
        }

        if(alpha_h == 0)
        {
            // expecting 0 output, set hX == 0
            for(rocblas_int b = 0; b < batch_count; b++)
                rocblas_init_zero((T*)hX[b], M, N, ldb);

            if(arg.pointer_mode_host)
            {
                if(arg.unit_check)
                    unit_check_general<T>(M, N, ldb, hX, hXorB_1, batch_count);
                if(arg.norm_check)
                    err_host
                        = std::abs(norm_check_general<T>('F', M, N, ldb, hX, hXorB_1, batch_count));
            }

            if(arg.pointer_mode_device)
            {
                CHECK_HIP_ERROR(hXorB_1.transfer_from(dXorB));
                if(arg.unit_check)
                    unit_check_general<T>(M, N, ldb, hX, hXorB_1, batch_count);
                if(arg.norm_check)
                    err_device
                        = std::abs(norm_check_general<T>('F', M, N, ldb, hX, hXorB_1, batch_count));
            }
        }
        else
        {
            //computed result is in hx_or_b, so forward error is E = hx - hx_or_b
            // calculate vector-induced-norm 1 of matrix E
            for(int b = 0; b < batch_count; b++)
            {
                if(arg.pointer_mode_host)
                {
                    double err_host_batch
                        = rocblas_abs(matrix_norm_1<T>(M, N, ldb, hX[b], hXorB_1[b]));

                    if(arg.unit_check)
                        trsm_err_res_check<T>(err_host_batch, M, error_eps_multiplier, eps);

                    // hx_or_b contains A * (calculated X), so res = A * (calculated x) - b = hx_or_b - hb
                    cblas_trmm<T>(
                        side, uplo, transA, diag, M, N, 1.0 / alpha_h, hA[b], lda, hXorB_1[b], ldb);
                    // calculate vector-induced-norm 1 of matrix res
                    double err_host_res_batch
                        = rocblas_abs(matrix_norm_1<T>(M, N, ldb, hXorB_1[b], hB[b]));

                    if(arg.unit_check)
                        trsm_err_res_check<T>(err_host_res_batch, M, residual_eps_multiplier, eps);
                    err_host = std::max(std::max(err_host_batch, err_host_res_batch), err_host);
                }

                if(arg.pointer_mode_device)
                {
                    CHECK_HIP_ERROR(hXorB_1.transfer_from(dXorB));
                    double err_device_batch
                        = rocblas_abs(matrix_norm_1<T>(M, N, ldb, hX[b], hXorB_1[b]));

                    if(arg.unit_check)
                        trsm_err_res_check<T>(err_device_batch, M, error_eps_multiplier, eps);

                    cblas_trmm<T>(
                        side, uplo, transA, diag, M, N, 1.0 / alpha_h, hA[b], lda, hXorB_1[b], ldb);
                    double err_device_res_batch
                        = rocblas_abs(matrix_norm_1<T>(M, N, ldb, hXorB_1[b], hB[b]));

                    if(arg.unit_check)
                        trsm_err_res_check<T>(
                            err_device_res_batch, M, residual_eps_multiplier, eps);
                    err_device
                        = std::max(std::max(err_device_batch, err_device_res_batch), err_device);
                }
            }
        }
    }

    // can't use external interface with device memory as set up with
    // internal tests/offsets, so ensuring not used when internal_api is set.
    if(arg.timing && !internal_api)
    {
        int number_cold_calls = arg.cold_iters;
        int number_hot_calls  = arg.iters;

        // GPU rocBLAS
        CHECK_HIP_ERROR(dXorB.transfer_from(hXorB_1));

        CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_host));

        hipStream_t stream;
        CHECK_ROCBLAS_ERROR(rocblas_get_stream(handle, &stream));
        gpu_time_used = get_time_us_sync(stream); // in microseconds

        for(int i = 0; i < number_cold_calls; i++)
        {
            CHECK_ROCBLAS_ERROR(rocblas_trsm_batched_fn(handle,
                                                        side,
                                                        uplo,
                                                        transA,
                                                        diag,
                                                        M,
                                                        N,
                                                        &alpha_h,
                                                        dA.ptr_on_device(),
                                                        lda,
                                                        dXorB.ptr_on_device(),
                                                        ldb,
                                                        batch_count));
        }

        gpu_time_used = get_time_us_sync(stream);

        for(int i = 0; i < number_hot_calls; i++)
        {
            CHECK_ROCBLAS_ERROR(rocblas_trsm_batched_fn(handle,
                                                        side,
                                                        uplo,
                                                        transA,
                                                        diag,
                                                        M,
                                                        N,
                                                        &alpha_h,
                                                        dA.ptr_on_device(),
                                                        lda,
                                                        dXorB.ptr_on_device(),
                                                        ldb,
                                                        batch_count));
        }

        gpu_time_used = get_time_us_sync(stream) - gpu_time_used;

        // CPU cblas
        cpu_time_used = get_time_us_no_sync();

        for(int b = 0; b < batch_count; b++)
            cblas_trsm<T>(side, uplo, transA, diag, M, N, alpha_h, hA[b], lda, hB[b], ldb);

        cpu_time_used = get_time_us_no_sync() - cpu_time_used;

        ArgumentModel<e_side,
                      e_uplo,
                      e_transA,
                      e_diag,
                      e_M,
                      e_N,
                      e_alpha,
                      e_lda,
                      e_ldb,
                      e_batch_count>{}
            .log_args<T>(rocblas_cout,
                         arg,
                         gpu_time_used,
                         trsm_gflop_count<T>(M, N, K),
                         ArgumentLogging::NA_value,
                         cpu_time_used,
                         err_host,
                         err_device);
    }
}
