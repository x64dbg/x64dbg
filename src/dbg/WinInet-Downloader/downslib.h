/* Created by John Åkerblom 10/26/2014 */

#pragma once

#include <wchar.h>
#include <stdbool.h>

enum class downslib_error
{
    ok,
    createfile,
    inetopen,
    openurl,
    statuscode,
    cancel,
    incomplete
};

typedef bool (*downslib_cb)(void* userdata, unsigned long long read_bytes, unsigned long long total_bytes);

downslib_error downslib_download(const char* url,
                                 const wchar_t* filename,
                                 const char* useragent = "downslib",
                                 unsigned int timeout = 3000,
                                 downslib_cb cb = nullptr,
                                 void* userdata = nullptr);
