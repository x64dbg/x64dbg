#include <windows.h>

static HANDLE gGoEvent = nullptr;
static volatile LONG gMainCounter = 0;
static volatile LONG gWorkerCounter = 0;

extern "C" __declspec(dllexport) __declspec(noinline) void MainThreadOuterHit()
{
    InterlockedIncrement(&gMainCounter);
}

extern "C" __declspec(dllexport) __declspec(noinline) void WorkerThreadInnerHit()
{
    InterlockedExchangeAdd(&gWorkerCounter, 2);
}

static DWORD WINAPI WorkerThreadProc(void*)
{
    WaitForSingleObject(gGoEvent, INFINITE);
    WorkerThreadInnerHit();
    return 0;
}

int main()
{
    gGoEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if(gGoEvent == nullptr)
        return 1;

    auto thread = CreateThread(nullptr, 0, WorkerThreadProc, nullptr, 0, nullptr);
    if(thread == nullptr)
    {
        CloseHandle(gGoEvent);
        return 1;
    }

    MainThreadOuterHit();
    SetEvent(gGoEvent);

    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
    CloseHandle(gGoEvent);
    return 0;
}
