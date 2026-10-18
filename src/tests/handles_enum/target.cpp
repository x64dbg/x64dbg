#include <Windows.h>

#include <vector>

extern "C"
{
    __declspec(dllexport) HANDLE HighHandle = nullptr;

    __declspec(dllexport) __declspec(noinline) void HandlesReady()
    {
        Sleep(1);
    }
}

int main()
{
    // Keep enough handles open that the final value cannot fit in a USHORT.
    std::vector<HANDLE> events;
    for(size_t i = 0; i < 17000; i++)
    {
        const auto event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if(!event)
            return 1;
        events.push_back(event);
    }
    HighHandle = events.back();
    HandlesReady();
    for(const auto event : events)
        CloseHandle(event);
    return 0;
}
