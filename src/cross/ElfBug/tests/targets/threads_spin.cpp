// Four workers spin on their own ts_counters slot.
#include <pthread.h>
#include <csignal>
#include <ctime>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

extern "C"
{
    volatile unsigned long long ts_counters[4] = {0, 0, 0, 0};
    volatile int ts_stop = 0;
    // Each worker calls this once.
    void ts_worker_started(int index);
    // Called every 2^20 increments.
    void ts_worker_tick();
    // Main thread only.
    void ts_tick();
    // Labels main's call to ts_tick.
    void ts_call_tick();
    void ts_call_site();

    // Set to 1 to make main fault each loop.
    volatile int ts_fault_armed = 0;
    void* ts_fault_target = nullptr;
    void ts_fault();
    // Labels the faulting store.
    void ts_fault_site();
    // Thread the SIGSEGV handler ran on.
    volatile int ts_fault_handler_tid = 0;
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

    .globl ts_call_tick
    .type  ts_call_tick, @function
ts_call_tick:
    subq    $8, %rsp
    .globl ts_call_site
ts_call_site:
    call    ts_tick@PLT
    addq    $8, %rsp
    ret
)");

extern "C" void ts_worker_started(const int index)
{
    (void)index;
}

extern "C" void ts_worker_tick()
{
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
        {
            ts_counters[index] = ts_counters[index] + 1;
            if((ts_counters[index] & 0xFFFFF) == 0)
                ts_worker_tick();
        }
        return nullptr;
    }
}

namespace
{
    long pageSize = 4096;

    // Unprotects the page so the store succeeds on retry.
    void onFault(int)
    {
        ts_fault_handler_tid = static_cast<int>(syscall(SYS_gettid));
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
        ts_call_tick();
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
