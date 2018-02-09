/* Created by John Åkerblom 10/26/2014 */

#include "downslib.h"

#include <windows.h>
#include <stdio.h>
#include <Wininet.h>

#pragma comment(lib, "Wininet.lib")

#define DEFAULT_TIMEOUT 3000
#define DEFAULT_USERAGENT "downslib"

static int _download(const char* url,
                     const wchar_t* filename,
                     const char* useragent,
                     unsigned int timeout,
                     downslib_cb cb)
{
    HINTERNET hInternet = NULL;
    HINTERNET hUrl = NULL;
    HANDLE hFile = NULL;
    DWORD dwWritten = 0;
    DWORD dwFlags = 0;
    DWORD dwError = 0;
    DWORD dwRead = 0;
    DWORD dwLen = 0;
    BOOL bRet = FALSE;
    unsigned long long read_bytes = 0;
    unsigned long long total_bytes = 0;
    char buffer[2048];
    int ret = DOWNSLIB_ERROR_OK;

    hFile = CreateFileW(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    if(hFile == INVALID_HANDLE_VALUE)
    {
        ret = DOWNSLIB_ERROR_CREATEFILE;
        goto cleanup;
    }

    hInternet = InternetOpenA(useragent,
                              INTERNET_OPEN_TYPE_PRECONFIG,
                              NULL,
                              NULL,
                              0);

    if(hInternet == NULL)
    {
        return DOWNSLIB_ERROR_INETOPEN;
    }

    /* Set timeout as specified */
    InternetSetOptionA(hInternet,
                       INTERNET_OPTION_RECEIVE_TIMEOUT,
                       &timeout,
                       sizeof(timeout));

    dwFlags = INTERNET_FLAG_RELOAD;
    if(strncmp(url, "https://", 8) == 0)
    {
        dwFlags |= INTERNET_FLAG_SECURE;
        //dwFlags |= INTERNET_FLAG_IGNORE_CERT_DATE_INVALID;
        //dwFlags |= INTERNET_FLAG_IGNORE_CERT_CN_INVALID;
    }

    if((hUrl = InternetOpenUrlA(hInternet, url, NULL, 0, dwFlags, 0)) == NULL)
    {
        ret = DOWNSLIB_ERROR_OPENURL;
        goto cleanup;
    }

    /* Get HTTP content length */
    dwLen = sizeof(buffer);
    if(HttpQueryInfoA(hUrl, HTTP_QUERY_CONTENT_LENGTH, buffer, &dwLen, 0))
    {
        if(sscanf_s(buffer, "%llu", &total_bytes) != 1)
            total_bytes = 0;
    }

    /* Get HTTP status code */
    dwLen = sizeof(buffer);
    if(HttpQueryInfoA(hUrl, HTTP_QUERY_STATUS_CODE, buffer, &dwLen, 0))
    {
        int status_code = 0;
        if(sscanf_s(buffer, "%d", &status_code) != 1)
            status_code = 500;
        if(status_code != 200)
        {
            ret = DOWNSLIB_ERROR_STATUSCODE;
            SetLastError(status_code);
            goto cleanup;
        }
    }

    while(InternetReadFile(hUrl, buffer, sizeof(buffer), &dwRead))
    {
        read_bytes += dwRead;

        /* We are done if nothing more to read, so now we can report total size in our final cb call */
        if(dwRead == 0)
        {
            total_bytes = read_bytes;
        }

        /* Call the callback to report progress and cancellation */
        if(cb && !cb(read_bytes, total_bytes))
        {
            ret = DOWNSLIB_ERROR_CANCEL;
            goto cleanup;
        }

        /* Exit if nothing more to read */
        if(dwRead == 0)
        {
            break;
        }

        WriteFile(hFile, buffer, dwRead, &dwWritten, NULL);
    }

    if(total_bytes > 0 && read_bytes != total_bytes)
    {
        ret = DOWNSLIB_ERROR_INCOMPLETE;
    }

cleanup:
    if(hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
    if(hUrl != NULL)
        InternetCloseHandle(hUrl);
    if(hInternet != NULL)
        InternetCloseHandle(hInternet);

    return ret;
}

int downslib_download(const char* url,
                      const wchar_t* filename,
                      const char* useragent,
                      unsigned int timeout,
                      downslib_cb cb)
{
    return _download(url,
                     filename,
                     useragent ? useragent : DEFAULT_USERAGENT,
                     timeout ? timeout : DEFAULT_TIMEOUT,
                     cb);
}

int downslib_easy_download(const char* url, const wchar_t* filename)
{
    return downslib_download(url,
                             filename,
                             DEFAULT_USERAGENT,
                             DEFAULT_TIMEOUT,
                             NULL);
}
