#pragma once

#include <catch2/catch_test_macros.hpp>
#include "TestHarness.h"
#include "SymbolHelper.h"
#include <ElfBug/process/ProcFs.h>
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

#define FIXTURE(name) (std::string(ELFBUG_TESTS_TARGETS_DIR "/") + (name))

namespace ElfBug::test
{
    // Deletes a file or directory tree when the test ends, even if a REQUIRE fails.
    struct RemoveOnExit
    {
        std::filesystem::path path;

        ~RemoveOnExit()
        {
            std::error_code ignored;
            std::filesystem::remove_all(path, ignored);
        }
    };

    inline pid_t ParentPid(const pid_t pid)
    {
        const std::string status = procfs::ReadFile(procfs::Path(pid, "status"));
        return procfs::ParseNumber<pid_t>(procfs::FindValue(status, "PPid:")).value_or(0);
    }

    // Raw read: MemRead hides the patch byte these assertions are about.
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

    // The session's Process is gone once the detach completes, so read through /proc directly.
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

    // Running() goes true before execl() replaces the image; this waits for the real exec.
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

    inline pid_t WaitForClonedChild(const pid_t parent,
                                    const std::chrono::milliseconds timeout = std::chrono::seconds(5))
    {
        const auto start = std::chrono::steady_clock::now();
        while(std::chrono::steady_clock::now() - start < timeout)
        {
            for(const auto & entry : std::filesystem::directory_iterator("/proc"))
            {
                const pid_t pid = procfs::ParseNumber<pid_t>(entry.path().filename().string()).value_or(0);
                if(pid > 0 && pid != parent && ParentPid(pid) == parent)
                    return pid;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        return 0;
    }
}
