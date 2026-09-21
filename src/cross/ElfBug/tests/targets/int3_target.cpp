// Traps on purpose: the debugger must report the int3 and still deliver it to the handler.
#include <csignal>

extern "C"
{
    volatile int i3_handled = 0;
    volatile int i3_done = 0;
}

namespace
{
    void onTrap(int)
    {
        i3_handled = 1;
    }
}

int main()
{
    signal(SIGTRAP, onTrap);
    __asm__ volatile("int3");
    i3_done = 1;
    return i3_handled ? 0 : 1;
}
