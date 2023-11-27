#ifndef _TASKTHREAD_H
#define _TASKTHREAD_H

#include "_global.h"

#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>

const size_t TASK_THREAD_DEFAULT_SLEEP_TIME = 100;
template <typename F, typename... Args>
class TaskThread_
{
protected:
    F fn;
    std::tuple<Args...> args;
    bool active = true;
    HANDLE hThread;
    CRITICAL_SECTION access;
    HANDLE wakeupSemaphore;

    size_t minSleepTimeMs = 0;
    size_t wakeups = 0;
    size_t execs = 0;
    void Loop();

    // Given new args, compress it into old args.
    virtual std::tuple<Args...> CompressArguments(Args && ... args);

    // Reset called after we latch in a value
    virtual void ResetArgs() { }
public:
    void WakeUp(Args...);
    explicit TaskThread_(F, size_t minSleepTimeMs = TASK_THREAD_DEFAULT_SLEEP_TIME);
    virtual ~TaskThread_();
};

template <typename F>
class StringConcatTaskThread_ : public TaskThread_<F, std::string>
{
    virtual std::tuple<std::string> CompressArguments(std::string && msg) override
    {
        std::get<0>(this->args) += msg;
        return this->args;
    }

    // Reset called after we latch in a value
    void ResetArgs() override
    {
        std::get<0>(this->args).resize(0);
    }
public:
    explicit StringConcatTaskThread_(F fn, size_t minSleepTimeMs = TASK_THREAD_DEFAULT_SLEEP_TIME)
        : TaskThread_<F, std::string>(fn, minSleepTimeMs) {}
};

// using aliases for cleaner syntax
template<class T> using Invoke = typename T::type;

template<unsigned...> struct seq { using type = seq; };

template<class S1, class S2> struct concat;

template<unsigned... I1, unsigned... I2>
struct concat<seq<I1...>, seq<I2...>>
                                   : seq < I1..., (sizeof...(I1) + I2)... > {};

template<class S1, class S2> using Concat = Invoke<concat<S1, S2>>;

template<unsigned N> struct gen_seq;
template<unsigned N> using GenSeq = Invoke<gen_seq<N>>;

template<unsigned N>
struct gen_seq : Concat < GenSeq < N / 2 >, GenSeq < N - N / 2 >> {};

template<> struct gen_seq<0> : seq<> {};
template<> struct gen_seq<1> : seq<0> {};

#define DECLTYPE_AND_RETURN( eval ) -> decltype ( eval ) { return eval; }

template<typename F, typename Tuple, unsigned ...S>
auto apply_tuple_impl(F && fn, Tuple && t, const seq<S...> &)
DECLTYPE_AND_RETURN(std::forward<F>(fn)(std::get<S>(std::forward<Tuple>(t))...));

template<typename F, typename Tuple>
auto apply_from_tuple(F && fn, Tuple && t)
DECLTYPE_AND_RETURN(apply_tuple_impl(std::forward<F>(fn), std::forward<Tuple>(t),
                                     GenSeq <
                                     std::tuple_size<typename std::remove_reference<Tuple>::type>::value
                                     > ()));

template <typename F, typename... Args>
std::tuple<Args...> TaskThread_<F, Args...>::CompressArguments(Args && ... _args)
{
    return std::make_tuple<Args...>(std::forward<Args>(_args)...);
}

template <typename F, typename... Args> void TaskThread_<F, Args...>::WakeUp(Args... _args)
{
    ++this->wakeups;
    EnterCriticalSection(&this->access);
    this->args = CompressArguments(std::forward<Args>(_args)...);
    LeaveCriticalSection(&this->access);
    // This will fail silently if it's redundant, which is what we want.
    ReleaseSemaphore(this->wakeupSemaphore, 1, nullptr);
}

template <typename F, typename... Args> void TaskThread_<F, Args...>::Loop()
{
    std::tuple<Args...> argLatch;
    while(this->active)
    {
        WaitForSingleObject(this->wakeupSemaphore, INFINITE);

        EnterCriticalSection(&this->access);
        argLatch = this->args;
        this->ResetArgs();
        LeaveCriticalSection(&this->access);

        if(this->active)
        {
            apply_from_tuple(this->fn, argLatch);
            std::this_thread::sleep_for(std::chrono::milliseconds(this->minSleepTimeMs));
            ++this->execs;
        }
    }
}

template <typename F, typename... Args>
TaskThread_<F, Args...>::TaskThread_(F fn,
                                     size_t minSleepTimeMs) : fn(fn), minSleepTimeMs(minSleepTimeMs)
{
    this->wakeupSemaphore = CreateSemaphoreW(nullptr, 0, 1, nullptr);
    InitializeCriticalSection(&this->access);

    this->hThread = CreateThread(nullptr, 0, [](LPVOID thisPtr) -> DWORD
    {
        ((TaskThread_<F, Args...>*)thisPtr)->Loop();
        return 0;
    }, this, 0, nullptr);
    setThreadAffinityAllGroupCores(this->hThread);
}

template <typename F, typename... Args>
TaskThread_<F, Args...>::~TaskThread_()
{
    EnterCriticalSection(&this->access);
    this->active = false;
    LeaveCriticalSection(&this->access);
    ReleaseSemaphore(this->wakeupSemaphore, 1, nullptr);

    WaitForSingleObject(this->hThread, INFINITE);
    CloseHandle(this->hThread);

    DeleteCriticalSection(&this->access);
    CloseHandle(this->wakeupSemaphore);
}

#endif // _TASKTHREAD_H
