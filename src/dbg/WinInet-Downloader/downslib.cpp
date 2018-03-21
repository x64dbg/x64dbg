/* Created by John Åkerblom 10/26/2014 */

#include "downslib.h"

#include <windows.h>
#include <stdio.h>
#include <Wininet.h>
#include <functional>

#pragma comment(lib, "Wininet.lib")

class Cleanup
{
    const std::function<void()> & fn;
public:
    Cleanup(const std::function<void()> & fn) : fn(fn) { }
    ~Cleanup() { fn(); }
};

downslib_error downslib_download(const char* url,
                                 const wchar_t* filename,
                                 const char* useragent,
                                 unsigned int timeout,
                                 downslib_cb cb)
{
    HINTERNET hInternet = nullptr;
    HINTERNET hUrl = nullptr;
    HANDLE hFile = nullptr;

    Cleanup cleanup([&]()
    {
        DWORD dwLastError = GetLastError();
        if(hFile != INVALID_HANDLE_VALUE)
            CloseHandle(hFile);
        if(hUrl != NULL)
            InternetCloseHandle(hUrl);
        if(hInternet != NULL)
            InternetCloseHandle(hInternet);
        SetLastError(dwLastError);
    });

    hFile = CreateFileW(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    if(hFile == INVALID_HANDLE_VALUE)
        return downslib_error::createfile;

    hInternet = InternetOpenA(useragent,
                              INTERNET_OPEN_TYPE_PRECONFIG,
                              NULL,
                              NULL,
                              0);

    if(!hInternet)
        return downslib_error::inetopen;

    InternetSetOptionA(hInternet,
                       INTERNET_OPTION_RECEIVE_TIMEOUT,
                       &timeout,
                       sizeof(timeout));

    DWORD dwFlags = INTERNET_FLAG_RELOAD;
    if(strncmp(url, "https://", 8) == 0)
        dwFlags |= INTERNET_FLAG_SECURE;

    hUrl = InternetOpenUrlA(hInternet, url, NULL, 0, dwFlags, 0);
    if(!hUrl)
        return downslib_error::openurl;

    // Get HTTP content length
    char buffer[2048];
    DWORD dwLen = sizeof(buffer);
    unsigned long long total_bytes = 0;
    if(HttpQueryInfoA(hUrl, HTTP_QUERY_CONTENT_LENGTH, buffer, &dwLen, 0))
    {
        if(sscanf_s(buffer, "%llu", &total_bytes) != 1)
            total_bytes = 0;
    }

    // Get HTTP status code
    dwLen = sizeof(buffer);
    if(HttpQueryInfoA(hUrl, HTTP_QUERY_STATUS_CODE, buffer, &dwLen, 0))
    {
        int status_code = 0;
        if(sscanf_s(buffer, "%d", &status_code) != 1)
            status_code = 500;
        if(status_code != 200)
        {
            SetLastError(status_code);
            return downslib_error::statuscode;
        }
    }

    DWORD dwRead = 0;
    DWORD dwWritten = 0;
    unsigned long long read_bytes = 0;
    while(InternetReadFile(hUrl, buffer, sizeof(buffer), &dwRead))
    {
        read_bytes += dwRead;

        // We are done if nothing more to read, so now we can report total size in our final cb call
        if(dwRead == 0)
            total_bytes = read_bytes;

        // Call the callback to report progress and cancellation
        if(cb && !cb(read_bytes, total_bytes))
            return downslib_error::cancel;

        // Exit if nothing more to read
        if(dwRead == 0)
            break;

        WriteFile(hFile, buffer, dwRead, &dwWritten, nullptr);
    }

    if(total_bytes > 0 && read_bytes != total_bytes)
        return downslib_error::incomplete;

    return downslib_error::ok;
}