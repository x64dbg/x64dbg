// Sits in a sleep loop with a SIGUSR1 handler, so a test can prove a signal raised around
// attach time still reaches the process.
#include <csignal>
#include <ctime>

extern "C"
{
    volatile int sp_handled = 0;
    volatile int sp_stop = 0;
    volatile int sp_ready = 0;
}

namespace
{
    void onUsr1(int)
    {
        sp_handled = sp_handled + 1;
    }
}

int main()
{
    struct sigaction sa = {};
    sa.sa_handler = onUsr1;
    sigaction(SIGUSR1, &sa, nullptr);
    sp_ready = 1;

    while(sp_stop == 0)
    {
        timespec ts{0, 1000000};
        nanosleep(&ts, nullptr);
    }
    return 0;
}
