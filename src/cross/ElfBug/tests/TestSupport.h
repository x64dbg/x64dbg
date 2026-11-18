#pragma once

#include <catch2/catch_test_macros.hpp>
#include "TestHarness.h"
#include "SymbolHelper.h"
#include <ElfBug/elf/ElfImage.h>
#include <ElfBug/process/ProcFs.h>
#include <ElfBug/process/TraceeMemory.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <future>
#include <optional>
#include <string>
#include <sys/uio.h>
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

    // The T at address in pid, read with no Process.
    template<typename T>
    std::optional<T> ReadTraceeValue(const pid_t pid, const ElfBug::ptr address)
    {
        T value{};
        if(ReadTraceeMemory(pid, address, &value, sizeof(value)) != static_cast<ssize_t>(sizeof(value)))
            return std::nullopt;
        return value;
    }

    template<typename T>
    bool WriteTraceeValue(const pid_t pid, const ElfBug::ptr address, T value)
    {
        iovec local{&value, sizeof(value)};
        iovec remote{reinterpret_cast<void*>(address), sizeof(value)};
        return process_vm_writev(pid, &local, 1, &remote, 1, 0) == static_cast<ssize_t>(sizeof(value));
    }

    // Unlike MemRead, sees breakpoint bytes.
    inline std::optional<std::uint8_t> ReadProcessByte(const ElfBug::Process* process, const ElfBug::ptr address)
    {
        return process ? ReadTraceeValue<std::uint8_t>(process->pid, address) : std::nullopt;
    }

    inline bool WaitForProcessByte(const ElfBug::Process* process, const ElfBug::ptr address, const std::uint8_t expected,
                                   const std::chrono::milliseconds timeout = std::chrono::seconds(1))
    {
        return WaitUntil([&] { return ReadProcessByte(process, address) == expected; }, timeout);
    }

    // Reads by pid, so it also works after a detach.
    inline bool WaitForTraceeValue(const pid_t pid, const ElfBug::ptr address, const int expected,
                                   const std::chrono::milliseconds timeout = std::chrono::seconds(2))
    {
        return WaitUntil([&] { return ReadTraceeValue<int>(pid, address) == expected; }, timeout);
    }

    // Until pid runs path with its aux vector in place, so its symbols resolve.
    inline bool WaitForExeced(const pid_t pid, const std::string & path,
                              const std::chrono::milliseconds timeout = std::chrono::seconds(2))
    {
        return WaitUntil([&] { return ReadExecAuxv(pid, path, AT_ENTRY).has_value(); }, timeout);
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
        return WaitUntil([&] { return pred(ReadSpinSlots(process, counters)); }, timeout);
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
        return WaitUntil([&] { return pred(TaskState(pid, tid)); }, timeout);
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
        pid_t child = 0;
        const bool found = WaitUntil([&]
        {
            for(const auto & entry : std::filesystem::directory_iterator("/proc"))
            {
                child = procfs::ParseNumber<pid_t>(entry.path().filename().string()).value_or(0);
                if(child > 0 && child != parent && ElfBug::ParentPid(child) == parent)
                    return true;
            }
            return false;
        }, timeout);
        return found ? child : 0;
    }

    // The row holding address, from MemoryMap::Pages or the C API export.
    template<typename Page>
    const Page & RequirePage(const std::vector<Page> & pages, const uint64_t address)
    {
        const auto found = std::find_if(pages.begin(), pages.end(), [&](const Page & page)
        {
            return address >= page.base && address - page.base < page.size;
        });
        REQUIRE(found != pages.end());
        return *found;
    }

    inline std::vector<std::byte> ReadFileBytes(const std::string & path)
    {
        const std::string text = procfs::ReadFile(path);
        const auto* data = reinterpret_cast<const std::byte*>(text.data());
        return {data, data + text.size()};
    }

    inline std::optional<ElfBug::ElfImage> ParseBytes(const std::vector<std::byte> & bytes,
            const ElfBug::ElfImage::Parts parts = ElfBug::ElfImage::Parts::HeadersAndSections)
    {
        return ElfBug::ElfImage::Parse([&bytes](const uint64_t offset, void* buffer, const size_t size)
        {
            if(offset > bytes.size() || size > bytes.size() - offset)
                return false;
            std::memcpy(buffer, bytes.data() + offset, size);
            return true;
        }, parts);
    }

    inline bool HasSection(const ElfBug::ElfImage & image, const std::string & name)
    {
        const auto & sections = image.Sections();
        return std::any_of(sections.begin(), sections.end(), [&](const ElfBug::ElfSection & section)
        {
            return section.name == name;
        });
    }

    inline bool MapFilesOpenable()
    {
        const std::string line = procfs::ReadLine("/proc/self/maps");
        const auto first = procfs::ParseMapsLine(line);
        if(!first)
            return false;
        char path[96];
        std::snprintf(path, sizeof(path), "/proc/self/map_files/%" PRIx64 "-%" PRIx64, first->start, first->end);
        const int fd = open(path, O_RDONLY | O_CLOEXEC);
        if(fd == -1)
            return false;
        close(fd);
        return true;
    }
}
