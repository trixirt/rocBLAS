/* ************************************************************************
 * Copyright (C) 2022-2023 Advanced Micro Devices, Inc. All rights reserved.
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

#ifdef WIN32
#include <windows.h>

#else
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#endif

#include <stdlib.h>

#include "host_alloc.hpp"
#include "rocblas_test.hpp"

//!
//! @brief Memory free helper.  Returns kB or -1 if unknown.
//!
ptrdiff_t host_bytes_available()
{
#ifdef WIN32

    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    return (ptrdiff_t)status.ullAvailPhys;

#else

    const int BUF_MAX = 1024;
    char      buf[BUF_MAX];

    ptrdiff_t n_bytes = -1; // unknown

    FILE* fp = popen("cat /proc/meminfo", "r");
    if(fp == NULL)
    {
        return n_bytes;
    }

    static const char* mem_token     = "MemFree";
    static auto*       mem_free_type = getenv("ROCBLAS_CLIENT_ALLOC_AVAILABLE");
    if(mem_free_type)
    {
        mem_token = "MemAvail"; // MemAvailable
    }
    int mem_token_len = strlen(mem_token);

    while(fgets(buf, BUF_MAX, fp) != NULL)
    {
        // set env ROCBLAS_CLIENT_ALLOC_AVAILABLE to use MemAvailable if too many SKIPS occur
        if(!strncmp(buf, mem_token, mem_token_len))
        {
            sscanf(buf, "%*s %td", &n_bytes); // kB assumed as 3rd column and ignored
            n_bytes *= 1024;
            break;
        }
    }

    int status = pclose(fp);
    if(status == -1)
    {
        return -1;
    }
    else
    {
        return n_bytes;
    }

#endif
}

inline bool host_mem_safe(size_t n_bytes)
{
#if defined(ROCBLAS_BENCH)
    return true; // roll out to rocblas-bench when CI does perf testing
#else
    static auto* no_alloc_check = getenv("ROCBLAS_CLIENT_NO_ALLOC_CHECK");
    if(no_alloc_check)
    {
        return true;
    }

    constexpr size_t threshold = 100 * 1024 * 1024; // 100 MB
    if(n_bytes > threshold)
    {
        ptrdiff_t avail_bytes = host_bytes_available(); // negative if unknown
        if(avail_bytes >= 0 && n_bytes > avail_bytes)
        {
            rocblas_cerr << "Warning: skipped allocating " << n_bytes << " bytes ("
                         << (n_bytes >> 30) << " GB) as more than free memory ("
                         << (avail_bytes >> 30) << " GB)" << std::endl;

            // we don't try if it looks to push load into swap
            return false;
        }
    }
    return true;
#endif
}

void* host_malloc(size_t size)
{
    if(host_mem_safe(size))
    {
        void* ptr = malloc(size);

        static int value = -1;

        static auto once = false;
        if(!once)
        {
            auto* alloc_byte_str = getenv("ROCBLAS_CLIENT_ALLOC_FILL_HEX_BYTE");
            if(alloc_byte_str)
            {
                value = strtol(alloc_byte_str, nullptr, 16); // hex
            }
            once = true;
        }

        if(value != -1 && ptr)
            memset(ptr, value, size);

        return ptr;
    }
    else
        return nullptr;
}

void* host_calloc(size_t nmemb, size_t size)
{
    if(host_mem_safe(nmemb * size))
        return calloc(nmemb, size);
    else
        return nullptr;
}
