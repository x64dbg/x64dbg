// Hits its own int3 and handles the SIGTRAP.
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
