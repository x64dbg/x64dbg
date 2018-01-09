/* Created by John Åkerblom 10/26/2014 */

#include "downslib.h"

#include <windows.h>
#include <Wininet.h>

#pragma comment(lib, "Wininet.lib")

#define DEFAULT_TIMEOUT 3000
#define DEFAULT_USERAGENT "downslib"

char* _strchr(const char* str, char c)
{
    const char* p;

    for(p = str; *p != '\0'; p = AnsiNext(p))
    {
        if(*p == c)
            return (char*)p;
    }

    return NULL;
}

static const char* _path_from_url(const char* url)
{
    const char* path = NULL;

    /* First look for a dot to get to domain part of URL */
    path = _strchr(url, '.');

    if(path != NULL)
        ++path;
    else
        path = url;

    path = _strchr(path, '/');

    return path;
}

static int _get_content_length(const char* host,
                               const char* object,
                               int use_ssl,
                               const char* useragent,
                               unsigned int timeout,
                               int* total_bytes)
{
    HINTERNET hInternet = NULL;
    HINTERNET hConnection = NULL;
    HINTERNET hRequest = NULL;
    DWORD dwRead = 0;
    DWORD dwLen = 0;
    DWORD dwFlags = 0;
    /* */
    int ret = 0;

    hInternet = InternetOpenA(useragent,
                              INTERNET_OPEN_TYPE_PRECONFIG,
                              NULL,
                              NULL,
                              0);

    if(hInternet == NULL)
    {
        return 1;
    }

    InternetSetOptionA(hInternet,
                       INTERNET_OPTION_RECEIVE_TIMEOUT,
                       &timeout,
                       sizeof(DWORD));

    hConnection = InternetConnectA(hInternet,
                                   host,
                                   INTERNET_DEFAULT_HTTP_PORT,
                                   NULL,
                                   NULL,
                                   INTERNET_SERVICE_HTTP,
                                   0,
                                   0);

    dwFlags = INTERNET_FLAG_RELOAD;
    if(use_ssl)
    {
        dwFlags |= INTERNET_FLAG_SECURE;
        dwFlags |= INTERNET_FLAG_IGNORE_CERT_DATE_INVALID;
        dwFlags |= INTERNET_FLAG_IGNORE_CERT_CN_INVALID;
    }

    hRequest = HttpOpenRequestA(hConnection,
                                "HEAD",
                                object,
                                NULL,
                                NULL,
                                NULL,
                                dwFlags,
                                0);

    if(HttpSendRequestA(hRequest, NULL, 0, NULL, 0) == FALSE)
    {
        if(GetLastError() == ERROR_INTERNET_INVALID_CA && use_ssl)
        {
            dwFlags = 0;
            dwLen = sizeof(dwFlags);
            InternetQueryOptionA(hRequest, INTERNET_OPTION_SECURITY_FLAGS, (LPVOID)&dwFlags, &dwLen);
            dwFlags |= SECURITY_FLAG_IGNORE_UNKNOWN_CA;
            InternetSetOptionA(hRequest, INTERNET_OPTION_SECURITY_FLAGS, &dwFlags, sizeof(dwFlags));
        }
    }

    dwLen = sizeof(*total_bytes);
    if(HttpQueryInfoA(hRequest,
                      HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER,
                      (LPVOID)total_bytes,
                      &dwLen,
                      0) == FALSE)
    {
        ret = 2;
    }

    if(hRequest != NULL)
        InternetCloseHandle(hRequest);
    if(hConnection != NULL)
        InternetCloseHandle(hConnection);
    if(hInternet != NULL)
        InternetCloseHandle(hInternet);

    return ret;
}

static int _download(const char* url,
                     const char* host,
                     const char* object,
                     int use_ssl,
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
    /* */
    int read_bytes = 0;
    int total_bytes = 0;
    char buffer[2048];
    int ret = 0;

    /* Try to retrieve content-length, not essential */
    _get_content_length(host,
                        object,
                        use_ssl,
                        useragent,
                        timeout,
                        &total_bytes);

    hInternet = InternetOpenA(useragent,
                              INTERNET_OPEN_TYPE_PRECONFIG,
                              NULL,
                              NULL,
                              0);

    if(hInternet == NULL)
    {
        return 1;
    }

    dwFlags = INTERNET_FLAG_RELOAD;
    if(use_ssl)
    {
        dwFlags |= INTERNET_FLAG_SECURE;
        dwFlags |= INTERNET_FLAG_IGNORE_CERT_DATE_INVALID;
        dwFlags |= INTERNET_FLAG_IGNORE_CERT_CN_INVALID;
    }

    if((hUrl = InternetOpenUrlA(hInternet, url, NULL, 0, dwFlags, 0)) == NULL)
    {
        ret = 2;
        goto cleanup;
    }

    hFile = CreateFileW(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    if(hFile == NULL)
    {
        ret = 3;
        goto cleanup;
    }

    while(InternetReadFile(hUrl, buffer, sizeof(buffer), &dwRead))
    {
        read_bytes += dwRead;

        /* We are done if nothing more to read, so now we can report total size in our final cb call */
        if(dwRead == 0)
        {
            total_bytes = read_bytes;
        }

        if(cb != NULL)
        {
            if(cb(read_bytes, total_bytes) != 0)
            {
                ret = 4;
                goto cleanup;
            }
        }

        /* Exit if nothing more to read */
        if(dwRead == 0)
        {
            break;
        }

        WriteFile(hFile, buffer, dwRead, &dwWritten, NULL);
        /* FlushFileBuffers(hFile); */
    }

    if(total_bytes > 0 && read_bytes != total_bytes)
    {
        ret = 5;
    }

cleanup:
    if(hFile != NULL)
        CloseHandle(hFile);
    if(hUrl != NULL)
        InternetCloseHandle(hUrl);
    if(hInternet != NULL)
        InternetCloseHandle(hInternet);

    return ret;
}

static int _host_and_object_from_url(const char* url, char** host, const char** object)
{
    const char* object_tmp = _path_from_url(url);
    char* past_protocol = NULL;
    char* host_tmp = NULL;
    int ret = 0;
    int len = 0;

    // Point past the protocol part of URL, e.g past "HTTP://"
    past_protocol = _strchr(url, '/');
    if(past_protocol == NULL)
    {
        // Malformed url
        return 1;
    }
    past_protocol += 2; // Move past the two slashes in protocol specification

    if(object_tmp == NULL)
    {
        len = lstrlenA(past_protocol) + 1;
    }
    else
    {
        len = object_tmp - past_protocol + 1;
    }

    host_tmp = (char*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len + 1);
    lstrcpynA(host_tmp, past_protocol, len);

    *host = host_tmp;
    *object = object_tmp;

    return 0;
}

static void _free_host(char* host)
{
    HeapFree(GetProcessHeap(), 0, host);
}

int downslib_download(const char* url,
                      const wchar_t* filename,
                      int use_ssl,
                      const char* useragent,
                      unsigned int timeout,
                      downslib_cb cb)
{
    const char* object = NULL;
    char* host = NULL;
    int ret = 0;

    ret = _host_and_object_from_url(url, &host, &object);
    if(ret != 0)
    {
        return ret;
    }

    ret = _download(url,
                    host,
                    object,
                    use_ssl,
                    filename,
                    useragent ? useragent : DEFAULT_USERAGENT,
                    timeout ? timeout : DEFAULT_TIMEOUT,
                    cb);

    _free_host(host);

    return ret;
}

int downslib_easy_download(const char* url, const wchar_t* filename, int use_ssl)
{
    return downslib_download(url,
                             filename,
                             use_ssl,
                             DEFAULT_USERAGENT,
                             DEFAULT_TIMEOUT,
                             NULL);
}
