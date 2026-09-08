// One worker calls _exit while the others hot-loop through a breakpoint site, so a stop
// sweep runs with the leader already dying.
#include <pthread.h>
#include <unistd.h>
#include <ctime>

extern "C"
{
    // The debugger writes 1 here to start the exit.
    volatile int er_exit_now = 0;
    // Breakpoint site: the spinners pass through it constantly.
    void er_hot();
}

extern "C" void er_hot()
{
}

namespace
{
    void nap()
    {
        timespec ts{0, 100000};
        nanosleep(&ts, nullptr);
    }

    void* exiter(void*)
    {
        while(er_exit_now == 0)
            nap();
        _exit(7);
        return nullptr;
    }

    void* spinner(void*)
    {
        for(;;)
            er_hot();
        return nullptr;
    }
}

int main()
{
    pthread_t threads[4];
    pthread_create(&threads[0], nullptr, exiter, nullptr);
    for(int i = 1; i < 4; ++i)
        pthread_create(&threads[i], nullptr, spinner, nullptr);

    for(;;)
        nap();
    return 0;
}
