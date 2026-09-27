// Spinners hit a breakpoint while another thread raises signals at itself.
#include <pthread.h>
#include <csignal>
#include <unistd.h>
#include "TargetUtil.h"

extern "C"
{
    volatile int sr_signal = 0;
    volatile int sr_quota = 0;
    volatile int sr_go = 0;
    volatile int sr_raised = 0;
    volatile int sr_handled = 0;
    volatile int sr_done = 0;
    void sr_hot();
}

extern "C" void sr_hot()
{
}

namespace
{
    void onSignal(int)
    {
        sr_handled = sr_handled + 1;
    }

    // sr_raised counts raises, sr_handled counts deliveries.
    void* raiser(void*)
    {
        while(sr_go == 0)
            nap(100000);
        while(sr_raised < sr_quota)
        {
            raise(sr_signal);
            sr_raised = sr_raised + 1;
        }
        sr_done = 1;
        for(;;)
            nap(100000);
        return nullptr;
    }

    void* spinner(void*)
    {
        for(;;)
            sr_hot();
        return nullptr;
    }
}

int main()
{
    struct sigaction sa = {};
    sa.sa_handler = onSignal;
    sigaction(SIGUSR1, &sa, nullptr);
    sigaction(SIGSEGV, &sa, nullptr);
    sigaction(SIGTRAP, &sa, nullptr);

    // Spinners first, so their traps are reaped before the raiser's stop.
    pthread_t threads[3];
    for(int i = 0; i < 2; ++i)
        pthread_create(&threads[i], nullptr, spinner, nullptr);
    pthread_create(&threads[2], nullptr, raiser, nullptr);

    for(;;)
        nap(100000);
    return 0;
}
