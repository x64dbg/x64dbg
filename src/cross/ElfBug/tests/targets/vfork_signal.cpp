#include <sys/wait.h>
#include <unistd.h>
#include <csignal>
#include <ctime>

extern "C"
{
    volatile int vs_handled = 0;
    volatile int vs_code = 0;
    volatile int vs_pid = 0;
    volatile int vs_value = 0;
}

namespace
{
    void onUsr1(int, siginfo_t* info, void*)
    {
        vs_code = info->si_code;
        vs_pid = info->si_pid;
        vs_value = info->si_value.sival_int;
        vs_handled = vs_handled + 1;
    }
}

int main()
{
    struct sigaction sa = {};
    sa.sa_sigaction = onUsr1;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGUSR1, &sa, nullptr);

    const pid_t child = vfork();
    if(child == 0)
    {
        const timespec ts{10, 0};
        nanosleep(&ts, nullptr);
        _exit(0);
    }
    waitpid(child, nullptr, 0);

    for(;;)
        pause();
}
