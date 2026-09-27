// Threads raise SIGUSR1 nonstop, so an attach often catches a signal stop.
#include <pthread.h>
#include <atomic>
#include <csignal>

extern "C"
{
    volatile int ss_stop = 0;
    std::atomic<int> ss_handled{0};
}

namespace
{
    void onUsr1(int)
    {
        ss_handled.fetch_add(1, std::memory_order_relaxed);
    }

    void* worker(void*)
    {
        while(ss_stop == 0)
            raise(SIGUSR1);
        return nullptr;
    }
}

int main()
{
    struct sigaction sa = {};
    sa.sa_handler = onUsr1;
    sigaction(SIGUSR1, &sa, nullptr);

    pthread_t threads[8];
    for(auto & t : threads)
    {
        if(pthread_create(&t, nullptr, worker, nullptr) != 0)
            return 1;
    }

    for(auto & t : threads)
        pthread_join(t, nullptr);
    return 0;
}
