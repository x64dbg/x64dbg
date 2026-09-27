#pragma once

#include <catch2/catch_test_macros.hpp>
#include "TestHarness.h"
#include "SymbolHelper.h"
#include <ElfBug/process/ProcFs.h>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <future>
#include <optional>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

#define FIXTURE(name) (std::string(ELFBUG_TESTS_TARGETS_DIR "/") + (name))

namespace ElfBug::test
{
    struct RemoveOnExit
    {
        std::filesystem::path path;

        ~RemoveOnExit()
        {
            std::error_code ignored;
            std::filesystem::remove_all(path, ignored);
        }
    };

    // Unlike MemRead, sees breakpoint bytes.
    inline std::optional<std::uint8_t> ReadProcessByte(const ElfBug::Process* process, const ElfBug::ptr address)
    {
        if(!process)
            return std::nullopt;

        std::uint8_t byte = 0;
        if(!process->MemReadRaw(address, &byte, 1))
            return std::nullopt;
        return byte;
    }

    inline bool WaitForProcessByte(const ElfBug::Process* process, const ElfBug::ptr address, const std::uint8_t expected,
                                   const std::chrono::milliseconds timeout = std::chrono::seconds(1))
    {
        const auto start = std::chrono::steady_clock::now();
        while(std::chrono::steady_clock::now() - start < timeout)
        {
            const auto byte = ReadProcessByte(process, address);
            if(byte && *byte == expected)
                return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return false;
    }

    inline bool WaitForTraceeValue(const ElfBug::Process* process, const ElfBug::ptr address,
                                   const int expected,
                                   const std::chrono::milliseconds timeout = std::chrono::seconds(2))
    {
        const auto start = std::chrono::steady_clock::now();
        while(std::chrono::steady_clock::now() - start < timeout)
        {
            int value = 0;
            if(process && process->MemReadRaw(address, &value, sizeof(value)) && value == expected)
                return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return false;
    }

    // Reads /proc directly, since the Process is gone after a detach.
    inline bool WaitForDetachedValue(const pid_t pid, const ElfBug::ptr address, const int expected,
                                     const std::chrono::milliseconds timeout = std::chrono::seconds(2))
    {
        const auto start = std::chrono::steady_clock::now();
        while(std::chrono::steady_clock::now() - start < timeout)
        {
            std::ifstream mem(procfs::Path(pid, "mem"), std::ios::binary);
            if(mem)
            {
                mem.seekg(static_cast<std::streamoff>(address));
                int value = 0;
                if(mem.read(reinterpret_cast<char*>(&value), sizeof(value)) && value == expected)
                    return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return false;
    }

    inline bool WaitForExeced(const pid_t pid, const std::string & path,
                              const std::chrono::milliseconds timeout = std::chrono::seconds(2))
    {
        std::error_code ec;
        const auto want = std::filesystem::canonical(path, ec);
        if(ec)
            return false;
        const auto start = std::chrono::steady_clock::now();
        while(std::chrono::steady_clock::now() - start < timeout)
        {
            std::error_code linkEc;
            const auto have = std::filesystem::canonical(procfs::Path(pid, "exe"), linkEc);
            if(!linkEc && have == want)
                return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return false;
    }

    // threads_spin's ts_counters.
    using SpinSlots = std::array<std::uint64_t, 4>;

    inline SpinSlots ReadSpinSlots(const ElfBug::Process* process, const ElfBug::ptr counters)
    {
        SpinSlots slots{};
        REQUIRE(process->MemRead(counters, slots.data(), sizeof(slots)));
        return slots;
    }

    template<typename Pred>
    bool WaitForSpinSlots(const ElfBug::Process* process, const ElfBug::ptr counters, Pred pred,
                          const std::chrono::milliseconds timeout = std::chrono::seconds(5))
    {
        const auto start = std::chrono::steady_clock::now();
        while(std::chrono::steady_clock::now() - start < timeout)
        {
            if(pred(ReadSpinSlots(process, counters)))
                return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return false;
    }

    inline bool WaitForEverySlotPast(const ElfBug::Process* process, const ElfBug::ptr counters, const SpinSlots & since)
    {
        return WaitForSpinSlots(process, counters, [&](const SpinSlots & now)
        {
            for(std::size_t i = 0; i < now.size(); ++i)
            {
                if(now[i] <= since[i])
                    return false;
            }
            return true;
        });
    }

    // 't' is ptrace-stop.
    inline char TaskState(const pid_t pid, const pid_t tid)
    {
        const auto fields = procfs::StatFields(procfs::ReadFile(procfs::TaskPath(pid, tid, "stat")));
        if(fields.size() <= procfs::kStatState || fields[procfs::kStatState].empty())
            return '?';
        return fields[procfs::kStatState].front();
    }

    template<typename Pred>
    bool WaitForTaskState(const pid_t pid, const pid_t tid, Pred pred,
                          const std::chrono::milliseconds timeout = std::chrono::seconds(5))
    {
        const auto start = std::chrono::steady_clock::now();
        while(std::chrono::steady_clock::now() - start < timeout)
        {
            if(pred(TaskState(pid, tid)))
                return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return false;
    }

    inline bool WaitForTaskStopped(const pid_t pid, const pid_t tid)
    {
        return WaitForTaskState(pid, tid, [](const char state) { return state == 't'; });
    }

    inline bool WaitForTaskRunning(const pid_t pid, const pid_t tid)
    {
        return WaitForTaskState(pid, tid, [](const char state) { return state == 'R' || state == 'S'; });
    }

    inline void RequireEveryTaskStopped(const pid_t pid)
    {
        std::vector<pid_t> tids;
        REQUIRE(ElfBug::ReadTaskList(pid, tids));
        REQUIRE_FALSE(tids.empty());
        for(const pid_t tid : tids)
        {
            CAPTURE(tid);
            REQUIRE(TaskState(pid, tid) == 't');
        }
    }

    // Only meaningful from OnDetach.
    inline std::vector<pid_t> TracedTasks(const pid_t pid)
    {
        std::vector<pid_t> tids;
        std::vector<pid_t> traced;
        ElfBug::ReadTaskList(pid, tids);
        for(const pid_t tid : tids)
        {
            const std::string status = procfs::ReadFile(procfs::TaskPath(pid, tid, "status"));
            if(procfs::ParseNumber<pid_t>(procfs::FindValue(status, "TracerPid:")).value_or(0) != 0)
                traced.push_back(tid);
        }
        return traced;
    }

    inline pid_t WaitForClonedChild(const pid_t parent,
                                    const std::chrono::milliseconds timeout = std::chrono::seconds(5))
    {
        const auto start = std::chrono::steady_clock::now();
        while(std::chrono::steady_clock::now() - start < timeout)
        {
            for(const auto & entry : std::filesystem::directory_iterator("/proc"))
            {
                const pid_t pid = procfs::ParseNumber<pid_t>(entry.path().filename().string()).value_or(0);
                if(pid > 0 && pid != parent && ElfBug::ParentPid(pid) == parent)
                    return pid;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        return 0;
    }
}
