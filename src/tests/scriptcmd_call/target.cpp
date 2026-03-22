#include <windows.h>

static volatile int gValue = 0;

extern "C" __declspec(dllexport) __declspec(noinline) void OuterHit()
{
    gValue += 1;
}

extern "C" __declspec(dllexport) __declspec(noinline) void InnerHit()
{
    gValue += 2;
}

int main()
{
    OuterHit();
    InnerHit();
    return gValue == 3 ? 0 : 1;
}
