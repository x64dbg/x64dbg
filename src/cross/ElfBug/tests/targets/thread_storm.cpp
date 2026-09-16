// A multithreaded target for exercising the attach sweep against real, running threads.
#include <pthread.h>
#include <ctime>

extern "C"
{
    volatile int tst_stop = 0;
}

namespace
{
    void nap(const long nanos)
    {
        timespec ts{0, nanos};
        nanosleep(&ts, nullptr);
    }

    void* worker(void*)
    {
        while(tst_stop == 0)
            nap(1000000);
        return nullptr;
    }
}

int main()
{
    pthread_t threads[32];
    for(auto & t : threads)
    {
        // Exit loudly rather than joining an uninitialised pthread_t below.
        if(pthread_create(&t, nullptr, worker, nullptr) != 0)
            return 1;
        nap(300000);
    }

    while(tst_stop == 0)
        nap(1000000);

    for(auto & t : threads)
        pthread_join(t, nullptr);
    return 0;
}
