// Raises, queues and faults signals in the mode argv[1] names, for the signal filter tests.
#include <pthread.h>
#include <csetjmp>
#include <csignal>
#include <cerrno>
#include <cstring>
#include <initializer_list>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <ucontext.h>
#include <unistd.h>

extern "C"
{
    volatile int flt_handled[65] = {};
    volatile int flt_code = 0;
    volatile int flt_pid = 0;
    volatile int flt_value = 0;
    volatile unsigned long flt_spin[2] = {};
    volatile int flt_go = 0;
    void flt_ready();
    void flt_step_site();
    void flt_loop_site();
    void flt_callee();
    void flt_call_from();
    void flt_in_handler();
    void flt_on_fault(int signal, siginfo_t* info, void* context);
    void flt_fault();
    void flt_group_exit();
    void* flt_bad_stack = nullptr;
    void flt_pushf();
    void flt_restorer();
    void flt_on_pushf_fault(int signal, siginfo_t* info, void* context);
    void flt_done();
}

// Whole functions in asm, so each symbol sits on exactly the instruction the tests need.
asm(R"(
    .pushsection .text
    .globl flt_step_site
    .type flt_step_site, @function
flt_step_site:
    nop
    nop
    nop
    nop
    nop
    ret
    .size flt_step_site, .-flt_step_site

    .globl flt_call_from
    .type flt_call_from, @function
flt_call_from:
    sub $8, %rsp
    .globl flt_call_site
flt_call_site:
    call flt_callee
    add $8, %rsp
    ret
    .size flt_call_from, .-flt_call_from

    .globl flt_fault
    .type flt_fault, @function
flt_fault:
    xor %eax, %eax
    .globl flt_fault_site
flt_fault_site:
    movl $1, (%rax)
    ret
    .size flt_fault, .-flt_fault

    .globl flt_group_exit
    .type flt_group_exit, @function
flt_group_exit:
    mov $231, %eax
    mov $5, %edi
    .globl flt_group_exit_site
flt_group_exit_site:
    syscall
    .size flt_group_exit, .-flt_group_exit

    .globl flt_pushf
    .type flt_pushf, @function
flt_pushf:
    push %r12
    mov %rsp, %r12
    mov flt_bad_stack(%rip), %rsp
    .globl flt_pushf_site
flt_pushf_site:
    pushfq
    mov %r12, %rsp
    pop %r12
    ret
    .size flt_pushf, .-flt_pushf

    .balign 512
    .fill 256, 1, 0xcc
    .globl flt_restorer
    .type flt_restorer, @function
flt_restorer:
    mov $15, %eax
    syscall
    .size flt_restorer, .-flt_restorer
    .popsection
)");

extern "C" void flt_ready()
{
}

extern "C" void flt_loop_site()
{
}

extern "C" void flt_callee()
{
}

extern "C" void flt_in_handler()
{
}

extern "C" void flt_done()
{
}

namespace
{
    constexpr int kRealtimeSignal = 40;
    constexpr int kBurstCount = 16;
    constexpr int kTimerTicks = 300;
    constexpr suseconds_t kTimerInterval = 1000;
    constexpr int kLoopThreads = 3;
    constexpr int kLoopIterations = 5;

    sigjmp_buf faultReturn;
    volatile bool exitInHandler = false;
    volatile bool workerStop = false;

    void record(const int signal, const siginfo_t* info)
    {
        flt_code = info->si_code;
        flt_pid = info->si_pid;
        flt_value = info->si_value.sival_int;
        flt_handled[signal] = flt_handled[signal] + 1;
        flt_in_handler();
    }

    void onSignal(const int signal, siginfo_t* info, void*)
    {
        if(signal == SIGUSR1 && exitInHandler)
            _exit(7);
        record(signal, info);
    }

    void install(const int signal, void (*handler)(int, siginfo_t*, void*), const int flags)
    {
        struct sigaction sa = {};
        sa.sa_sigaction = handler;
        sa.sa_flags = SA_SIGINFO | flags;
        sigaction(signal, &sa, nullptr);
    }

    void installAll(const int flags)
    {
        for(const int signal : {SIGUSR1, SIGUSR2, SIGALRM, SIGCHLD, kRealtimeSignal})
            install(signal, onSignal, flags);
        install(SIGSEGV, flt_on_fault, flags);
    }

    void useAltStack()
    {
        static char stack[64 * 1024];
        stack_t ss = {};
        ss.ss_sp = stack;
        ss.ss_size = sizeof(stack);
        sigaltstack(&ss, nullptr);
    }

    void* worker(void*)
    {
        while(!workerStop)
            flt_spin[1] = flt_spin[1] + 1;
        return nullptr;
    }

    pthread_t startWorker()
    {
        pthread_t thread;
        pthread_create(&thread, nullptr, worker, nullptr);
        return thread;
    }

    void stopWorker(const pthread_t thread)
    {
        workerStop = true;
        pthread_join(thread, nullptr);
    }

    void setTimer(const suseconds_t interval)
    {
        const itimerval timer{{0, interval}, {0, interval}};
        setitimer(ITIMER_REAL, &timer, nullptr);
    }

    pthread_barrier_t loopStart;

    void* loopWorker(void*)
    {
        pthread_barrier_wait(&loopStart);
        for(int i = 0; i < kLoopIterations; ++i)
            flt_loop_site();
        return nullptr;
    }

    void runThreads()
    {
        pthread_barrier_init(&loopStart, nullptr, kLoopThreads);
        pthread_t threads[kLoopThreads];
        for(pthread_t & thread : threads)
            pthread_create(&thread, nullptr, loopWorker, nullptr);
        for(const pthread_t thread : threads)
            pthread_join(thread, nullptr);
        pthread_barrier_destroy(&loopStart);
    }

    void* groupExitingWorker(void*)
    {
        flt_group_exit();
        return nullptr;
    }

    // One thread ends the process with exit_group(5) while the other is alive: the worker when byWorker, else main.
    void runGroupExit(const bool byWorker)
    {
        pthread_t thread;
        pthread_create(&thread, nullptr, byWorker ? groupExitingWorker : worker, nullptr);
        if(byWorker)
            pthread_join(thread, nullptr);
        while(flt_spin[1] == 0)
        {
        }
        flt_group_exit();
    }

    void runSites()
    {
        flt_step_site();
        for(int i = 0; i < 3; ++i)
            flt_loop_site();
        flt_call_from();
    }

    void runBurst()
    {
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, kRealtimeSignal);
        sigprocmask(SIG_BLOCK, &set, nullptr);
        for(int i = 0; i < kBurstCount; ++i)
        {
            sigval value{};
            value.sival_int = i;
            sigqueue(getpid(), kRealtimeSignal, value);
        }
        sigprocmask(SIG_UNBLOCK, &set, nullptr);
    }

    void runTimer()
    {
        const pthread_t thread = startWorker();
        setTimer(kTimerInterval);
        while(flt_handled[SIGALRM] < kTimerTicks)
            flt_spin[0] = flt_spin[0] + 1;
        setTimer(0);
        stopWorker(thread);
    }

    void runTimerLoop()
    {
        const pthread_t thread = startWorker();
        setTimer(kTimerInterval);
        while(flt_go != 1)
        {
            flt_spin[0] = flt_spin[0] + 1;
            flt_loop_site();
        }
        setTimer(0);
        stopWorker(thread);
    }

    void runChild()
    {
        const pid_t child = fork();
        if(child == 0)
            _exit(0);
        while(waitpid(child, nullptr, 0) == -1 && errno == EINTR)
        {
        }
    }

    void runWait()
    {
        const pthread_t thread = startWorker();
        while(flt_go != 1)
            flt_spin[0] = flt_spin[0] + 1;
        stopWorker(thread);
    }

    void runFault()
    {
        if(sigsetjmp(faultReturn, 1) == 0)
            flt_fault();
    }

    // Faults on a pushfq with the stack pointer on an unwritable page; the handler runs on the alternate stack.
    void runPushf()
    {
        struct KernelSigaction
        {
            void (*handler)(int, siginfo_t*, void*);
            unsigned long flags;
            void (*restorer)();
            unsigned long mask;
        };
        constexpr unsigned long kRestorer = 0x04000000;
        const auto installRaw = [](const int signal, void (*handler)(int, siginfo_t*, void*))
        {
            const KernelSigaction action{handler, SA_SIGINFO | SA_ONSTACK | kRestorer, flt_restorer, 0};
            syscall(SYS_rt_sigaction, signal, &action, nullptr, sizeof(action.mask));
        };
        useAltStack();
        installRaw(SIGSEGV, flt_on_pushf_fault);
        installRaw(SIGUSR1, onSignal);
        flt_bad_stack = static_cast<char*>(mmap(nullptr, 4096, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)) + 4096;
        flt_pushf();
    }

    bool is(const char* mode, const char* name)
    {
        return std::strcmp(mode, name) == 0;
    }
}

extern "C" void flt_on_fault(const int signal, siginfo_t* info, void*)
{
    record(signal, info);
    siglongjmp(faultReturn, 1);
}

extern "C" void flt_on_pushf_fault(const int signal, siginfo_t* info, void* context)
{
    auto* user = static_cast<ucontext_t*>(context);
    user->uc_mcontext.gregs[REG_RSP] = user->uc_mcontext.gregs[REG_R12];
    user->uc_mcontext.gregs[REG_RIP] += 1;
    record(signal, info);
}

int main(int argc, char** argv)
{
    const char* mode = argc > 1 ? argv[1] : "raise";
    const bool altstack = is(mode, "altstack");
    if(altstack)
        useAltStack();
    exitInHandler = is(mode, "exitinhandler");
    installAll(altstack ? SA_ONSTACK : 0);

    flt_ready();

    if(is(mode, "sites") || altstack || exitInHandler)
        runSites();
    else if(is(mode, "raise"))
        raise(SIGUSR1);
    else if(is(mode, "nohandler"))
    {
        signal(SIGSEGV, SIG_DFL);
        raise(SIGSEGV);
    }
    else if(is(mode, "burst"))
        runBurst();
    else if(is(mode, "timer"))
        runTimer();
    else if(is(mode, "timerloop"))
        runTimerLoop();
    else if(is(mode, "child"))
        runChild();
    else if(is(mode, "tstp"))
        raise(SIGTSTP);
    else if(is(mode, "fault"))
        runFault();
    else if(is(mode, "pushf"))
        runPushf();
    else if(is(mode, "wait"))
        runWait();
    else if(is(mode, "threads"))
        runThreads();
    else if(is(mode, "groupexit"))
        runGroupExit(argc > 2 && is(argv[2], "worker"));
    else
        return 2;

    flt_done();
    return 0;
}
