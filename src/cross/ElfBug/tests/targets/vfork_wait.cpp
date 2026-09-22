// The main thread spends almost all its time in vfork's uninterruptible wait, where SIGSTOP cannot stop it.
#include <sys/wait.h>
#include <unistd.h>
#include "TargetUtil.h"

extern "C"
{
    volatile int vw_rounds = 0;
}

int main()
{
    for(;;)
    {
        const pid_t child = vfork();
        if(child == 0)
        {
            nap(400000000);
            _exit(0);
        }
        waitpid(child, nullptr, 0);
        vw_rounds = vw_rounds + 1;
    }
}
