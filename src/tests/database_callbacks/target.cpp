#include <Windows.h>

#pragma section(".mbd3", read, write)

extern "C"
{
    __declspec(allocate(".mbd3")) __declspec(dllexport) volatile signed char VariableTarget = 1;

#pragma code_seg(push, database_cb_code, ".mbc")
    __declspec(dllexport) __declspec(noinline) void FunctionTarget()
    {
        return;
    }
}

int main()
{
    Sleep(1000);
};
#pragma code_seg(pop, membp_code)

