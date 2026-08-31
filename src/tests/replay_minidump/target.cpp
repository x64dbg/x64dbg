#include <Windows.h>
#include <DbgHelp.h>

#include <array>
#include <cstdio>

#pragma comment(lib, "dbghelp.lib")

extern "C" __declspec(dllexport) volatile ULONG_PTR replay_marker =
#ifdef _WIN64
    0x1122334455667788ull;
#else
    0x55667788u;
#endif

namespace
{

const wchar_t* gDumpPath = nullptr;
std::array<HANDLE, 2> gWorkerReady = {};
HANDLE gStopWorkers = nullptr;

DWORD WINAPI worker(void* argument)
{
    const auto index = reinterpret_cast<ULONG_PTR>(argument);
    SetEvent(gWorkerReady[index]);
    WaitForSingleObject(gStopWorkers, INFINITE);
    return static_cast<DWORD>(replay_marker + index);
}

LONG writeDump(EXCEPTION_POINTERS* exception)
{
    const auto file = CreateFileW(gDumpPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if(file == INVALID_HANDLE_VALUE)
        return EXCEPTION_EXECUTE_HANDLER;

    MINIDUMP_EXCEPTION_INFORMATION exceptionInfo = {};
    exceptionInfo.ThreadId = GetCurrentThreadId();
    exceptionInfo.ExceptionPointers = exception;
    exceptionInfo.ClientPointers = FALSE;
    const auto type = static_cast<MINIDUMP_TYPE>(
        MiniDumpWithFullMemory |
        MiniDumpWithFullMemoryInfo |
        MiniDumpWithThreadInfo |
        MiniDumpWithUnloadedModules |
        MiniDumpWithHandleData |
        MiniDumpIgnoreInaccessibleMemory);
    const auto written = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file, type,
                                           &exceptionInfo, nullptr, nullptr);
    const auto error = GetLastError();
    CloseHandle(file);
    if(!written)
    {
        DeleteFileW(gDumpPath);
        SetLastError(error);
    }
    return EXCEPTION_EXECUTE_HANDLER;
}

} // namespace

int wmain(int argc, wchar_t** argv)
{
    if(argc != 2)
    {
        std::fwprintf(stderr, L"usage: replay_minidump <output.dmp>\n");
        return 2;
    }
    gDumpPath = argv[1];
    gStopWorkers = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    std::array<HANDLE, 2> workers = {};
    for(size_t i = 0; i < workers.size(); ++i)
    {
        gWorkerReady[i] = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        workers[i] = CreateThread(nullptr, 0, worker, reinterpret_cast<void*>(i), 0, nullptr);
    }
    WaitForMultipleObjects(static_cast<DWORD>(gWorkerReady.size()), gWorkerReady.data(), TRUE, 10000);

    __try
    {
        const ULONG_PTR parameters[] = { 1, 0xDEADF00D };
        RaiseException(EXCEPTION_ACCESS_VIOLATION, 0, static_cast<DWORD>(std::size(parameters)), parameters);
    }
    __except(writeDump(GetExceptionInformation()))
    {
    }

    const auto dumpCreated = GetFileAttributesW(gDumpPath) != INVALID_FILE_ATTRIBUTES;
    SetEvent(gStopWorkers);
    WaitForMultipleObjects(static_cast<DWORD>(workers.size()), workers.data(), TRUE, 10000);
    for(const auto workerHandle : workers)
        CloseHandle(workerHandle);
    for(const auto ready : gWorkerReady)
        CloseHandle(ready);
    CloseHandle(gStopWorkers);
    return dumpCreated ? 0 : 1;
}
