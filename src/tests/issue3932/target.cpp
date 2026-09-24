#include <windows.h>

#include <cstdio>

int main()
{
    std::printf("ready %lu\n", GetCurrentProcessId());
    std::fflush(stdout);

    // Keep the process alive until the Python driver releases stdin.
    (void)std::getchar();
    return 0;
}
