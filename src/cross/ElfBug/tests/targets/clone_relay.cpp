// Each relay thread clones the next, so the cloner is always the newest thread.
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include "TargetUtil.h"

extern "C"
{
    volatile int cr_stop = 0;
    volatile int cr_relayed = 0;
}

namespace
{
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

    // The relay starts once an attach sweep has.
    bool leaderTraced()
    {
        char status[4096];
        const int fd = open("/proc/self/status", O_RDONLY | O_CLOEXEC);
        if(fd == -1)
            return false;
        const ssize_t n = read(fd, status, sizeof(status) - 1);
        close(fd);
        if(n <= 0)
            return false;
        status[n] = '\0';
        const char* tracer = strstr(status, "TracerPid:");
        return tracer && atoi(tracer + 10) != 0;
    }

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
        while(cr_relayed == 0 && !leaderTraced())
            nap(10000);

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
