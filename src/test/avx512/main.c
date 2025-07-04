#include <stdio.h>

__declspec(dllexport) void inspect_state()
{

}

extern void test_avx512();

int main()
{
    test_avx512();
}
