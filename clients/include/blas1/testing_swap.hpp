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

#include "bytes.hpp"
#include "cblas_interface.hpp"
#include "flops.hpp"
#include "norm.hpp"
#include "rocblas.hpp"
#include "rocblas_init.hpp"
#include "rocblas_math.hpp"
#include "rocblas_random.hpp"
#include "rocblas_test.hpp"
#include "rocblas_vector.hpp"
#include "unit.hpp"
#include "utility.hpp"

template <typename T>
void testing_swap_bad_arg(const Arguments& arg)
{
    auto rocblas_swap_fn = arg.api == FORTRAN ? rocblas_swap<T, true> : rocblas_swap<T, false>;

    rocblas_int N    = 100;
    rocblas_int incx = 1;
    rocblas_int incy = 1;

    rocblas_local_handle handle{arg};

    // Allocate device memory
    device_vector<T> dx(N, incx);
    device_vector<T> dy(N, incy);

    // Check device memory allocation
    CHECK_DEVICE_ALLOCATION(dx.memcheck());
    CHECK_DEVICE_ALLOCATION(dy.memcheck());

    EXPECT_ROCBLAS_STATUS(rocblas_swap_fn(nullptr, N, dx, incx, dy, incy),
                          rocblas_status_invalid_handle);
    EXPECT_ROCBLAS_STATUS(rocblas_swap_fn(handle, N, nullptr, incx, dy, incy),
                          rocblas_status_invalid_pointer);
    EXPECT_ROCBLAS_STATUS(rocblas_swap_fn(handle, N, dx, incx, nullptr, incy),
                          rocblas_status_invalid_pointer);
}

template <typename T>
void testing_swap(const Arguments& arg)
{
    auto rocblas_swap_fn = arg.api == FORTRAN ? rocblas_swap<T, true> : rocblas_swap<T, false>;

    rocblas_int          N    = arg.N;
    rocblas_int          incx = arg.incx;
    rocblas_int          incy = arg.incy;
    rocblas_local_handle handle{arg};

    // argument sanity check before allocating invalid memory
    if(N <= 0)
    {
        CHECK_ROCBLAS_ERROR(rocblas_swap_fn(handle, N, nullptr, incx, nullptr, incy));
        return;
    }

    // Naming: `h` is in CPU (host) memory(eg hx), `d` is in GPU (device) memory (eg dx).
    // Allocate host memory
    host_vector<T> hx(N, incx);
    host_vector<T> hy(N, incy);
    host_vector<T> hx_gold(N, incx);
    host_vector<T> hy_gold(N, incy);

    // Allocate device memory
    device_vector<T> dx(N, incx);
    device_vector<T> dy(N, incy);

    // Check device memory allocation
    CHECK_DEVICE_ALLOCATION(dx.memcheck());
    CHECK_DEVICE_ALLOCATION(dy.memcheck());

    // Initial Data on CPU
    rocblas_init_vector(hx, arg, rocblas_client_alpha_sets_nan, true);
    rocblas_init_vector(hy, arg, rocblas_client_alpha_sets_nan, false);

    // swap vector is easy in STL; hy_gold = hx: save a swap in hy_gold which will be output of CPU
    // BLAS
    hx_gold = hx;
    hy_gold = hy;

    // copy data from CPU to device
    CHECK_HIP_ERROR(dx.transfer_from(hx));
    CHECK_HIP_ERROR(dy.transfer_from(hy));

    double gpu_time_used, cpu_time_used;
    double rocblas_error = 0.0;

    if(arg.unit_check || arg.norm_check)
    {
        handle.pre_test(arg);
        // GPU BLAS
        CHECK_ROCBLAS_ERROR(rocblas_swap_fn(handle, N, dx, incx, dy, incy));
        handle.post_test(arg);
        CHECK_HIP_ERROR(hx.transfer_from(dx));
        CHECK_HIP_ERROR(hy.transfer_from(dy));

        // CPU BLAS
        cpu_time_used = get_time_us_no_sync();
        cblas_swap<T>(N, hx_gold, incx, hy_gold, incy);
        cpu_time_used = get_time_us_no_sync() - cpu_time_used;

        if(arg.unit_check)
        {
            unit_check_general<T>(1, N, incx, hx_gold, hx);
            unit_check_general<T>(1, N, incy, hy_gold, hy);
        }

        if(arg.norm_check)
        {
            rocblas_error = norm_check_general<T>('F', 1, N, incx, hx_gold, hx);
            rocblas_error = norm_check_general<T>('F', 1, N, incy, hy_gold, hy);
        }
    }

    if(arg.timing)
    {
        int number_cold_calls = arg.cold_iters;
        int number_hot_calls  = arg.iters;
        CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_host));

        for(int iter = 0; iter < number_cold_calls; iter++)
        {
            rocblas_swap_fn(handle, N, dx, incx, dy, incy);
        }

        hipStream_t stream;
        CHECK_ROCBLAS_ERROR(rocblas_get_stream(handle, &stream));
        gpu_time_used = get_time_us_sync(stream); // in microseconds

        for(int iter = 0; iter < number_hot_calls; iter++)
        {
            rocblas_swap_fn(handle, N, dx, incx, dy, incy);
        }

        gpu_time_used = get_time_us_sync(stream) - gpu_time_used;

        ArgumentModel<e_N, e_incx, e_incy>{}.log_args<T>(rocblas_cout,
                                                         arg,
                                                         gpu_time_used,
                                                         ArgumentLogging::NA_value,
                                                         swap_gbyte_count<T>(N),
                                                         cpu_time_used,
                                                         rocblas_error);
    }
}
