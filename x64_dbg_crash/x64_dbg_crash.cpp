#define _WIN32_WINNT 0x0500

#include <stdio.h>
#include <windows.h>
#include "x64_dbg_crash.h"
#include "..\x64_dbg_dbg\dbghelp\dbghelp.h"

static char szDumpPath[MAX_PATH]="";

static LONG WINAPI UnhandledException(EXCEPTION_POINTERS* pExceptionPointers)
{
    char szFileName[MAX_PATH];
#ifdef _WIN64
    const char* szAppName = "x64_dbg";
#else
    const char* szAppName = "x32_dbg";
#endif //_WIN64
    HANDLE hDumpFile;
    SYSTEMTIME stLocalTime;
    MINIDUMP_EXCEPTION_INFORMATION ExpParam;

    GetLocalTime( &stLocalTime );

    CreateDirectoryA(szDumpPath, 0);

    sprintf(szFileName, "%s\\%s-%04d%02d%02d-%02d%02d%02d-%ld-%ld.dmp",
            szDumpPath, szAppName,
            stLocalTime.wYear, stLocalTime.wMonth, stLocalTime.wDay,
            stLocalTime.wHour, stLocalTime.wMinute, stLocalTime.wSecond,
            GetCurrentProcessId(), GetCurrentThreadId());

    hDumpFile = CreateFile(szFileName, GENERIC_READ|GENERIC_WRITE,
                           FILE_SHARE_WRITE|FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);

    ExpParam.ThreadId = GetCurrentThreadId();
    ExpParam.ExceptionPointers = pExceptionPointers;
    ExpParam.ClientPointers = TRUE;

    MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
                      hDumpFile, MiniDumpWithDataSegs, &ExpParam, NULL, NULL);

    char szMessage[256]="";

    unsigned int ExceptionCode=pExceptionPointers->ExceptionRecord->ExceptionCode;

    sprintf(szMessage, "Exception code: 0x%.8X\n\nCrash dump written to:\n%s", ExceptionCode, szFileName);

    MessageBoxA(0, szMessage, "Fatal Exception!", MB_ICONERROR|MB_SYSTEMMODAL);

    ExitProcess(ExceptionCode);

    return EXCEPTION_EXECUTE_HANDLER;
}

__declspec(dllexport) void InitCrashHandler() //dummy function
{
}

extern "C" __declspec(dllexport) BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if(fdwReason==DLL_PROCESS_ATTACH)
    {
        int len=GetModuleFileNameA(hinstDLL, szDumpPath, MAX_PATH);
        while(szDumpPath[len]!='\\' && len)
            len--;
        if(len)
            szDumpPath[len]=0;
        strcat(szDumpPath, "\\crashdumps");
        AddVectoredExceptionHandler(1, UnhandledException);
    }
    return TRUE;
}
