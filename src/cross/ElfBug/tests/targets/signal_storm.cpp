// Several threads raise SIGUSR1 at themselves continuously, so an attach sweep is likely to
// observe the signal instead of a clean SIGSTOP on at least one of them.
#include <pthread.h>
#include <atomic>
#include <csignal>

extern "C"
{
    volatile int ss_stop = 0;
    // Eight handlers increment this at once, so a plain read-modify-write would lose counts.
    std::atomic<int> ss_handled{0};
}

namespace
{
    void onUsr1(int)
    {
        ss_handled.fetch_add(1, std::memory_order_relaxed);
    }

    // Unthrottled on purpose: the density is what makes the attach sweep land on a
    // signal-delivery-stop instead of a clean SIGSTOP, which is the whole fixture.
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
        // Exit loudly rather than joining an uninitialised pthread_t below.
        if(pthread_create(&t, nullptr, worker, nullptr) != 0)
            return 1;
    }

    for(auto & t : threads)
        pthread_join(t, nullptr);
    return 0;
}
