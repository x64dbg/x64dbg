#include <windows.h>

#include <array>
#include <cstdint>

struct ReplayTtdState
{
    volatile LONG stage;
    volatile LONG workerValues[3];
    volatile LONG repeatedCalls;
    volatile LONG helperValue;
    volatile LONG exceptionCount;
    volatile LONG finalValue;
};

extern "C" __declspec(dllexport) ReplayTtdState gReplayTtdState = {};

static HANDLE gWorkerStart[3] = {};
static HANDLE gWorkerDone[3] = {};

extern "C" __declspec(dllexport) __declspec(noinline)
LONG ReplayTtdKnownFunction(LONG worker, LONG iteration, LONG input)
{
    const LONG value = input * 3 + worker * 0x100 + iteration * 7 + 0x21;
    gReplayTtdState.workerValues[worker] = value;
    InterlockedIncrement(&gReplayTtdState.repeatedCalls);
    return value;
}

extern "C" __declspec(dllexport) __declspec(noinline)
void ReplayTtdMilestone(LONG stage, LONG value)
{
    gReplayTtdState.finalValue = value;
    InterlockedExchange(&gReplayTtdState.stage, stage);
}

static DWORD WINAPI ReplayTtdWorker(void* parameter)
{
    const LONG worker = static_cast<LONG>(reinterpret_cast<intptr_t>(parameter));
    if(WaitForSingleObject(gWorkerStart[worker], INFINITE) != WAIT_OBJECT_0)
        return 1;

    LONG value = 0x40 + worker;
    for(LONG iteration = 0; iteration < 8; iteration++)
        value = ReplayTtdKnownFunction(worker, iteration, value);

    ReplayTtdMilestone(10 + worker, value);
    SetEvent(gWorkerDone[worker]);
    return static_cast<DWORD>(value);
}

static LONG HandleReplayException(EXCEPTION_POINTERS* pointers)
{
    if(pointers && pointers->ExceptionRecord &&
       pointers->ExceptionRecord->ExceptionCode == 0xE0424242)
    {
        InterlockedIncrement(&gReplayTtdState.exceptionCount);
        return EXCEPTION_EXECUTE_HANDLER;
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

using HelperTransform = LONG(WINAPI*)(LONG);

int main()
{
    ReplayTtdMilestone(1, 0x11111111);

    std::array<HANDLE, 3> threads = {};
    for(LONG worker = 0; worker < 3; worker++)
    {
        gWorkerStart[worker] = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        gWorkerDone[worker] = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if(!gWorkerStart[worker] || !gWorkerDone[worker])
            return 10 + worker;
        threads[worker] = CreateThread(
            nullptr,
            0,
            ReplayTtdWorker,
            reinterpret_cast<void*>(static_cast<intptr_t>(worker)),
            0,
            nullptr);
        if(!threads[worker])
            return 20 + worker;
    }

    ReplayTtdMilestone(2, 0x22222222);
    HMODULE helper = LoadLibraryW(L"replay_ttd_helper.dll");
    if(!helper)
        return 30;
    auto transform = reinterpret_cast<HelperTransform>(GetProcAddress(helper, "ReplayTtdHelperTransform"));
#ifndef _WIN64
    if(!transform)
        transform = reinterpret_cast<HelperTransform>(GetProcAddress(helper, "_ReplayTtdHelperTransform@4"));
#endif
    if(!transform)
        return 31;

    gReplayTtdState.helperValue = transform(0x1234);
    ReplayTtdMilestone(3, gReplayTtdState.helperValue);

    // Start and finish workers in a fixed order. All three threads coexist in
    // the trace, while the state milestones remain deterministic.
    for(LONG worker = 0; worker < 3; worker++)
    {
        SetEvent(gWorkerStart[worker]);
        if(WaitForSingleObject(gWorkerDone[worker], INFINITE) != WAIT_OBJECT_0)
            return 40 + worker;
        ReplayTtdMilestone(20 + worker, gReplayTtdState.workerValues[worker]);
    }

    if(!FreeLibrary(helper))
        return 50;
    ReplayTtdMilestone(4, 0x44444444);

    __try
    {
        ULONG_PTR arguments[] = {0x545444, 0x424242};
        RaiseException(0xE0424242, 0, ARRAYSIZE(arguments), arguments);
    }
    __except(HandleReplayException(GetExceptionInformation()))
    {
        ReplayTtdMilestone(5, 0x55555555);
    }

    const LONG finalValue = ReplayTtdKnownFunction(0, 8, gReplayTtdState.workerValues[0]);
    ReplayTtdMilestone(6, finalValue);

    WaitForMultipleObjects(static_cast<DWORD>(threads.size()), threads.data(), TRUE, INFINITE);
    for(LONG worker = 0; worker < 3; worker++)
    {
        CloseHandle(threads[worker]);
        CloseHandle(gWorkerStart[worker]);
        CloseHandle(gWorkerDone[worker]);
    }

    if(gReplayTtdState.repeatedCalls != 25 ||
       gReplayTtdState.exceptionCount != 1 ||
       gReplayTtdState.stage != 6)
        return 60;
    return 0;
}
