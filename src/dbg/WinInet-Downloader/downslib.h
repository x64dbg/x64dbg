/* Created by John Åkerblom 10/26/2014 */

#ifndef __DOWNSLIB_H_DEF__
#define __DOWNSLIB_H_DEF__

#include <wchar.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

#define DOWNSLIB_ERROR_OK 0
#define DOWNSLIB_ERROR_CREATEFILE 1
#define DOWNSLIB_ERROR_INETOPEN 2
#define DOWNSLIB_ERROR_OPENURL 3
#define DOWNSLIB_ERROR_STATUSCODE 4
#define DOWNSLIB_ERROR_CANCEL 5
#define DOWNSLIB_ERROR_INCOMPLETE 6

int downslib_easy_download(const char* url, const wchar_t* filename);

typedef bool (*downslib_cb)(unsigned long long read_bytes, unsigned long long total_bytes);

int downslib_download(const char* url,
                      const wchar_t* filename,
                      const char* useragent,
                      unsigned int timeout,
                      downslib_cb cb);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif

