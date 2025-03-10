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
#include "rocblas_axpy.hpp"
#include "logging.hpp"
#include "rocblas_block_sizes.h"

namespace
{

    template <typename>
    constexpr char rocblas_axpy_name[] = "unknown";
    template <>
    constexpr char rocblas_axpy_name<float>[] = "rocblas_saxpy";
    template <>
    constexpr char rocblas_axpy_name<double>[] = "rocblas_daxpy";
    template <>
    constexpr char rocblas_axpy_name<rocblas_half>[] = "rocblas_haxpy";
    template <>
    constexpr char rocblas_axpy_name<rocblas_float_complex>[] = "rocblas_caxpy";
    template <>
    constexpr char rocblas_axpy_name<rocblas_double_complex>[] = "rocblas_zaxpy";

    template <typename T>
    rocblas_status rocblas_axpy_impl(rocblas_handle handle,
                                     rocblas_int    n,
                                     const T*       alpha,
                                     const T*       x,
                                     rocblas_int    incx,
                                     T*             y,
                                     rocblas_int    incy,
                                     const char*    name,
                                     const char*    bench_name)
    {
        if(!handle)
            return rocblas_status_invalid_handle;

        RETURN_ZERO_DEVICE_MEMORY_SIZE_IF_QUERIED(handle);

        auto layer_mode     = handle->layer_mode;
        auto check_numerics = handle->check_numerics;
        if(layer_mode & rocblas_layer_mode_log_trace)
            log_trace(handle, name, n, LOG_TRACE_SCALAR_VALUE(handle, alpha), x, incx, y, incy);

        if(layer_mode & rocblas_layer_mode_log_bench)
            log_bench(handle,
                      "./rocblas-bench",
                      "-f",
                      bench_name,
                      "-r",
                      rocblas_precision_string<T>,
                      "-n",
                      n,
                      LOG_BENCH_SCALAR_VALUE(handle, alpha),
                      "--incx",
                      incx,
                      "--incy",
                      incy);

        if(layer_mode & rocblas_layer_mode_log_profile)
            log_profile(handle, name, "N", n, "incx", incx, "incy", incy);

        static constexpr rocblas_int    batch_count_1 = 1;
        static constexpr rocblas_stride stride_0      = 0;
        static constexpr rocblas_stride offset_0      = 0;

        rocblas_status arg_status = rocblas_axpy_arg_check(handle,
                                                           n,
                                                           alpha,
                                                           x,
                                                           offset_0,
                                                           incx,
                                                           stride_0,
                                                           y,
                                                           offset_0,
                                                           incy,
                                                           stride_0,
                                                           batch_count_1);
        if(arg_status != rocblas_status_continue)
            return arg_status;

        if(check_numerics)
        {
            bool           is_input = true;
            rocblas_status axpy_check_numerics_status
                = rocblas_axpy_check_numerics(rocblas_axpy_name<T>,
                                              handle,
                                              n,
                                              x,
                                              offset_0,
                                              incx,
                                              stride_0,
                                              y,
                                              offset_0,
                                              incy,
                                              stride_0,
                                              batch_count_1,
                                              check_numerics,
                                              is_input);
            if(axpy_check_numerics_status != rocblas_status_success)
                return axpy_check_numerics_status;
        }

        rocblas_status status = rocblas_internal_axpy_template(handle,
                                                               n,
                                                               alpha,
                                                               stride_0,
                                                               x,
                                                               offset_0,
                                                               incx,
                                                               stride_0,
                                                               y,
                                                               offset_0,
                                                               incy,
                                                               stride_0,
                                                               batch_count_1);
        if(status != rocblas_status_success)
            return status;

        if(check_numerics)
        {
            bool           is_input = false;
            rocblas_status axpy_check_numerics_status
                = rocblas_axpy_check_numerics(rocblas_axpy_name<T>,
                                              handle,
                                              n,
                                              x,
                                              offset_0,
                                              incx,
                                              stride_0,
                                              y,
                                              offset_0,
                                              incy,
                                              stride_0,
                                              batch_count_1,
                                              check_numerics,
                                              is_input);
            if(axpy_check_numerics_status != rocblas_status_success)
                return axpy_check_numerics_status;
        }
        return status;
    }

}

/*
 * ===========================================================================
 *    C wrapper
 * ===========================================================================
 */

extern "C" {

#ifdef IMPL
#error IMPL ALREADY DEFINED
#endif

#define IMPL(routine_name_, T_)                                                               \
    rocblas_status routine_name_(rocblas_handle handle,                                       \
                                 rocblas_int    n,                                            \
                                 const T_*      alpha,                                        \
                                 const T_*      x,                                            \
                                 rocblas_int    incx,                                         \
                                 T_*            y,                                            \
                                 rocblas_int    incy)                                         \
    try                                                                                       \
    {                                                                                         \
        return rocblas_axpy_impl(handle, n, alpha, x, incx, y, incy, #routine_name_, "axpy"); \
    }                                                                                         \
    catch(...)                                                                                \
    {                                                                                         \
        return exception_to_rocblas_status();                                                 \
    }

IMPL(rocblas_saxpy, float);
IMPL(rocblas_daxpy, double);
IMPL(rocblas_caxpy, rocblas_float_complex);
IMPL(rocblas_zaxpy, rocblas_double_complex);
IMPL(rocblas_haxpy, rocblas_half);

#undef IMPL

} // extern "C"
