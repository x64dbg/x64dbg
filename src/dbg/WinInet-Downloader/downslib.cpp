/* Created by John Åkerblom 10/26/2014 */

#include "downslib.h"

#include <windows.h>
#include <stdio.h>
#include <Wininet.h>
#include <functional>

#pragma comment(lib, "Wininet.lib")

class Cleanup
{
    std::function<void()> fn;
public:
    explicit Cleanup(std::function<void()> fn) : fn(std::move(fn)) { }
    ~Cleanup() { fn(); }
};

downslib_error downslib_download(const char* url,
                                 const wchar_t* filename,
                                 const char* useragent,
                                 unsigned int timeout,
                                 downslib_cb cb,
                                 void* userdata)
{
    HINTERNET hInternet = nullptr;
    HINTERNET hUrl = nullptr;
    HANDLE hFile = nullptr;
    DWORD dwBufferSize = 0;
    const char* headers = nullptr;
    DWORD dwHeaderslen = 0;
    const DWORD chunk_size = 8192;

    Cleanup cleanup([&]()
    {
        DWORD dwLastError = GetLastError();
        if(hFile != INVALID_HANDLE_VALUE)
        {
            bool doDelete = false;
            LARGE_INTEGER fileSize;
            if(dwLastError != ERROR_SUCCESS || (GetFileSizeEx(hFile, &fileSize) && fileSize.QuadPart == 0))
            {
                // an error occurred and now there is an empty or incomplete file that didn't exist before we came in
                doDelete = true;
            }
            CloseHandle(hFile);
            if(doDelete)
                DeleteFileW(filename);
        }
        if(hUrl != NULL)
            InternetCloseHandle(hUrl);
        if(hInternet != NULL)
            InternetCloseHandle(hInternet);
        SetLastError(dwLastError);
    });

    // Create the PDB file that will be written to
    hFile = CreateFileW(filename, GENERIC_WRITE | FILE_READ_ATTRIBUTES, 0, NULL, CREATE_ALWAYS, 0, NULL);
    if(hFile == INVALID_HANDLE_VALUE)
        return downslib_error::createfile;

    hInternet = InternetOpenA(useragent,
                              INTERNET_OPEN_TYPE_PRECONFIG,
                              NULL,
                              NULL,
                              0);

    if(!hInternet)
    {
        CloseHandle(hFile);
        return downslib_error::inetopen;
    }

    // Set a time-out value in milliseconds
    InternetSetOptionA(hInternet,
                       INTERNET_OPTION_RECEIVE_TIMEOUT,
                       &timeout,
                       sizeof(timeout));

    // Attempt to enable content decoding if the option is supported
    DWORD gzipDecoding = true;
    if(InternetSetOptionA(hInternet,
                          INTERNET_OPTION_HTTP_DECODING,
                          &gzipDecoding,
                          sizeof(gzipDecoding)))
    {
        // Add the required request headers
        headers = "Accept-Encoding: gzip\r\n";
        dwHeaderslen = static_cast<DWORD>(strlen(headers));
    }

    DWORD dwFlags = INTERNET_FLAG_RELOAD;
    if(strncmp(url, "https://", 8) == 0)
        dwFlags |= INTERNET_FLAG_SECURE;

    // Make the HTTP request
    hUrl = InternetOpenUrlA(hInternet, url, headers, dwHeaderslen, dwFlags, 0);
    if(!hUrl)
        return downslib_error::openurl;

    // Get HTTP status code
    DWORD dwStatusCode;
    dwBufferSize = sizeof(dwStatusCode);
    if(HttpQueryInfoA(hUrl, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &dwStatusCode, &dwBufferSize, 0))
    {
        if(dwStatusCode != 200)
        {
            SetLastError(dwStatusCode);
            return downslib_error::statuscode;
        }
    }

    // Get HTTP content length
    unsigned long long contentLength = 0;
    dwBufferSize = sizeof(contentLength);
    HttpQueryInfoA(hUrl, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &contentLength, &dwBufferSize, 0);

    // Receive the data from the HINTERNET handle
    char buffer[chunk_size];
    DWORD dwRead = 0;
    DWORD dwWritten = 0;
    unsigned long long readBytes = 0;
    unsigned long long totalBytes = contentLength;
    while(InternetReadFile(hUrl, buffer, sizeof(buffer), &dwRead))
    {
        readBytes += dwRead;

        // We are done if nothing more to read, so now we can report total size in our final cb call
        if(dwRead == 0)
            totalBytes = readBytes;

        // Call the callback to report progress and cancellation
        if(cb && !cb(userdata, readBytes, totalBytes))
        {
            SetLastError(ERROR_OPERATION_ABORTED);
            return downslib_error::cancel;
        }

        // Exit if nothing more to read
        if(dwRead == 0)
            break;

        // Write buffer to PDB file
        WriteFile(hFile, buffer, dwRead, &dwWritten, nullptr);

        // Check WriteFile successfully wrote the entire buffer
        if(dwWritten != dwRead)
        {
            SetLastError(ERROR_IO_INCOMPLETE);
            return downslib_error::incomplete;
        }

    }

    if(totalBytes > 0 && readBytes != totalBytes)
    {
        SetLastError(ERROR_IO_INCOMPLETE);
        return downslib_error::incomplete;
    }

    SetLastError(ERROR_SUCCESS);
    return downslib_error::ok;
}