// Two spinners hot-loop through a breakpoint site, so every hit sweeps the others, while
// a third thread raises a signal at itself in a tight loop. Whichever raise the sweep
// absorbs instead of the main loop is the case under test.
#include <pthread.h>
#include <csignal>
#include <ctime>
#include <unistd.h>

extern "C"
{
    // The debugger writes the signal and quota, then sets sr_go.
    volatile int sr_signal = 0;
    volatile int sr_quota = 0;
    volatile int sr_go = 0;
    // Raises that returned, handler runs seen, and set once every raise has returned.
    volatile int sr_raised = 0;
    volatile int sr_handled = 0;
    volatile int sr_done = 0;
    // Breakpoint site: the spinners pass through it constantly.
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

    void nap()
    {
        constexpr timespec ts{0, 100000};
        nanosleep(&ts, nullptr);
    }

    // raise() returns only after the handler ran, or at once if the debugger suppressed
    // the signal, so sr_raised counts attempts and sr_handled counts deliveries.
    void* raiser(void*)
    {
        while(sr_go == 0)
            nap();
        while(sr_raised < sr_quota)
        {
            raise(sr_signal);
            sr_raised = sr_raised + 1;
        }
        sr_done = 1;
        for(;;)
            nap();
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

    // Spinners first: waitpid(-1) walks tracees in attach order, so their traps are seen
    // before the raiser's stop and the sweep is what finds it.
    pthread_t threads[3];
    for(int i = 0; i < 2; ++i)
        pthread_create(&threads[i], nullptr, spinner, nullptr);
    pthread_create(&threads[2], nullptr, raiser, nullptr);

    for(;;)
        nap();
    return 0;
}
