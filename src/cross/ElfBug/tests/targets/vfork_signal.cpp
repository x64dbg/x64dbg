#include <sys/wait.h>
#include <unistd.h>
#include <csignal>
#include <ctime>

extern "C"
{
    volatile int vs_handled = 0;
}

namespace
{
    void onUsr1(int)
    {
        vs_handled = vs_handled + 1;
    }
}

int main()
{
    struct sigaction sa = {};
    sa.sa_handler = onUsr1;
    sigaction(SIGUSR1, &sa, nullptr);

    const pid_t child = vfork();
    if(child == 0)
    {
        // Bounded so a failed test does not leave it behind.
        const timespec ts{10, 0};
        nanosleep(&ts, nullptr);
        _exit(0);
    }
    waitpid(child, nullptr, 0);

    for(;;)
        pause();
}
