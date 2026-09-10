// Long-lived workers, so a test can tell a frozen tracee from a running one by watching
// the counters. Each worker writes only its own slot.
#include <pthread.h>
#include <csignal>
#include <ctime>
#include <sys/mman.h>
#include <unistd.h>

extern "C"
{
    volatile unsigned long long ts_counters[4] = {0, 0, 0, 0};
    // The debugger writes 1 here to let the tracee exit cleanly.
    volatile int ts_stop = 0;
    // Breakpoint site: every worker passes through it exactly once.
    void ts_worker_started(int index);
    // Breakpoint site on the main thread, which touches no counter.
    void ts_tick();

    // The debugger writes 1 here to make the main thread fault once per loop.
    volatile int ts_fault_armed = 0;
    // Points at a page the main loop keeps read-only while the fault is armed.
    void* ts_fault_target = nullptr;
    void ts_fault();
    // Labels the faulting store, so a breakpoint can sit on it and the step off that byte
    // is what faults.
    void ts_fault_site();
}

asm(R"(
    .text

    .globl ts_fault
    .type  ts_fault, @function
ts_fault:
    movq    ts_fault_target(%rip), %rax
    .globl ts_fault_site
ts_fault_site:
    movl    $1, (%rax)
    ret
)");

extern "C" void ts_worker_started(const int index)
{
    (void)index;
}

extern "C" void ts_tick()
{
}

namespace
{
    void* worker(void* arg)
    {
        const auto index = static_cast<int>(reinterpret_cast<long>(arg));
        ts_worker_started(index);
        while(ts_stop == 0)
            ts_counters[index] = ts_counters[index] + 1;
        return nullptr;
    }
}

namespace
{
    long pageSize = 4096;

    // Make the store land on retry, so a reported fault does not kill the tracee.
    void onFault(int)
    {
        mprotect(ts_fault_target, static_cast<size_t>(pageSize), PROT_READ | PROT_WRITE);
    }
}

int main()
{
    pageSize = sysconf(_SC_PAGESIZE);
    if(pageSize <= 0)
        pageSize = 4096;
    ts_fault_target = mmap(nullptr, static_cast<size_t>(pageSize), PROT_READ,
                           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    struct sigaction sa = {};
    sa.sa_handler = onFault;
    sigaction(SIGSEGV, &sa, nullptr);

    pthread_t threads[4];
    for(long i = 0; i < 4; ++i)
        pthread_create(&threads[i], nullptr, worker, reinterpret_cast<void*>(i));

    while(ts_stop == 0)
    {
        ts_tick();
        if(ts_fault_armed)
        {
            mprotect(ts_fault_target, static_cast<size_t>(pageSize), PROT_READ);
            ts_fault();
        }
        timespec ts{0, 1000000};
        nanosleep(&ts, nullptr);
    }

    for(auto & t : threads)
        pthread_join(t, nullptr);
    return 0;
}
