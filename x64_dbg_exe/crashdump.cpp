#include <windows.h>
#include <dbghelp.h>
#include <stdio.h>
#include "crashdump.h"

BOOL
(WINAPI*
 MiniDumpWriteDumpPtr)(
     _In_ HANDLE hProcess,
     _In_ DWORD ProcessId,
     _In_ HANDLE hFile,
     _In_ MINIDUMP_TYPE DumpType,
     _In_opt_ PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
     _In_opt_ PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
     _In_opt_ PMINIDUMP_CALLBACK_INFORMATION CallbackParam
 );

void CrashDumpInitialize()
{
    // Find the DbgHelp module first
    HMODULE module = LoadLibrary("dbghelp.dll");

    if(module)
        *(FARPROC*)&MiniDumpWriteDumpPtr = GetProcAddress(module, "MiniDumpWriteDump");

    if(MiniDumpWriteDumpPtr)
        AddVectoredExceptionHandler(0, CrashDumpVectoredHandler);
}

void CrashDumpFatal(const char* Format, ...)
{
    char buffer[1024];
    va_list va;

    va_start(va, Format);
    vsnprintf_s(buffer, _TRUNCATE, Format, va);
    va_end(va);

    MessageBox(nullptr, buffer, "Error", MB_ICONERROR);
}

void CrashDumpCreate(EXCEPTION_POINTERS* ExceptionPointers)
{
    // Generate a crash dump file in the root directory
    wchar_t dumpFile[MAX_PATH];
    wchar_t dumpDir[MAX_PATH];

    if(!GetCurrentDirectoryW(ARRAYSIZE(dumpDir), dumpDir))
    {
        CrashDumpFatal("Unable to obtain current directory during crash dump\n");
        return;
    }
    wcscat_s(dumpDir, L"\\minidump");
    CreateDirectoryW(dumpDir, nullptr);

    // Append the name
    SYSTEMTIME st = {0};
    GetLocalTime(&st);
    swprintf_s(dumpFile, L"%ws\\dump-%02d%02d%04d_%02d%02d%02d.dmp", dumpDir,
               st.wDay,
               st.wMonth,
               st.wYear,
               st.wHour,
               st.wMinute,
               st.wSecond);

    // Open the file
    HANDLE fileHandle = CreateFileW(dumpFile, GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

    if(fileHandle == INVALID_HANDLE_VALUE)
    {
        CrashDumpFatal("Failed to open file path '%ws' while generating crash dump\n", dumpFile);
        return;
    }

    // Create the mini dump with DbgHelp
    MINIDUMP_EXCEPTION_INFORMATION info;
    memset(&info, 0, sizeof(MINIDUMP_EXCEPTION_INFORMATION));

    info.ThreadId = GetCurrentThreadId();
    info.ExceptionPointers = ExceptionPointers;
    info.ClientPointers = TRUE;

    if(!MiniDumpWriteDumpPtr(GetCurrentProcess(), GetCurrentProcessId(), fileHandle, MiniDumpNormal, &info, nullptr, nullptr))
        CrashDumpFatal("MiniDumpWriteDump failed. Error: %u\n", GetLastError());

    // Close the file & done
    CloseHandle(fileHandle);
}

LONG CALLBACK CrashDumpVectoredHandler(EXCEPTION_POINTERS* ExceptionInfo)
{
    if(ExceptionInfo)
    {
        // Skip DBG_PRINTEXCEPTION_C
        if(ExceptionInfo->ExceptionRecord->ExceptionCode != DBG_PRINTEXCEPTION_C)
            return EXCEPTION_CONTINUE_SEARCH;

        CrashDumpCreate(ExceptionInfo);
    }

    return EXCEPTION_CONTINUE_SEARCH;
}