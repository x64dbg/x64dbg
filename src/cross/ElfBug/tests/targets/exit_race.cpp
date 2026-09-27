// One worker exits while the others spin through a breakpoint.
#include <pthread.h>
#include <unistd.h>
#include "TargetUtil.h"

extern "C"
{
    volatile int er_exit_now = 0;
    void er_hot();
}

extern "C" void er_hot()
{
}

namespace
{
    void* exiter(void*)
    {
        while(er_exit_now == 0)
        {
        }
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
        nap(100000);
    return 0;
}
