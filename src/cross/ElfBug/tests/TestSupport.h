#pragma once

#include <catch2/catch_test_macros.hpp>
#include "TestHarness.h"
#include "SymbolHelper.h"
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
            std::ifstream mem("/proc/" + std::to_string(pid) + "/mem", std::ios::binary);
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
            const auto have = std::filesystem::canonical("/proc/" + std::to_string(pid) + "/exe", linkEc);
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
                const auto name = entry.path().filename().string();
                if(name.find_first_not_of("0123456789") != std::string::npos)
                    continue;
                const auto pid = static_cast<pid_t>(std::stol(name));
                if(pid != parent && ElfBug::ParentPid(pid) == parent)
                    return pid;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        return 0;
    }
}
