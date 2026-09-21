// The cloning thread is always the newest, so a sweep that freezes its first pass still
// leaves one free to clone behind its back. thread_storm clones from the leader.
#include <pthread.h>
#include <cstddef>
#include "TargetUtil.h"

extern "C"
{
    volatile int cr_stop = 0;
    // Hand-offs so far. Only the current relay thread writes it.
    volatile int cr_relayed = 0;
}

namespace
{
    // The parked threads give a sweep pass enough to do that hand-offs keep landing behind
    // it; the budget is what stops the relay and bounds the thread count.
    constexpr int kParked = 48;
    constexpr int kRelayLimit = 128;
    constexpr std::size_t kStackSize = 128 * 1024;

    void park()
    {
        while(cr_stop == 0)
            nap(1000000);
    }

    void* parked(void*)
    {
        park();
        return nullptr;
    }

    void* relay(void*);

    void spawn(void* (*entry)(void*))
    {
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setstacksize(&attr, kStackSize);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_t t;
        pthread_create(&t, &attr, entry, nullptr);
        pthread_attr_destroy(&attr);
    }

    void* relay(void*)
    {
        if(cr_stop == 0 && cr_relayed < kRelayLimit)
        {
            cr_relayed = cr_relayed + 1;
            spawn(relay);
        }
        park();
        return nullptr;
    }
}

int main()
{
    for(int i = 0; i < kParked; ++i)
        spawn(parked);
    spawn(relay);

    park();
    return 0;
}
