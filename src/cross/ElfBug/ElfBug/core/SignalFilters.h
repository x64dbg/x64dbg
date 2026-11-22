#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <string_view>

namespace ElfBug
{
    struct SignalFilter
    {
        bool stop = true;
        bool log = true;
        bool pass = true;
        bool operator==(const SignalFilter &) const = default;
    };

    class SignalFilters
    {
    public:
        static constexpr int kMaxSignal = 64;

        SignalFilters();
        // 1 to 64 except SIGKILL and SIGSTOP.
        [[nodiscard]] static bool Filterable(int signal);
        [[nodiscard]] static SignalFilter Default(int signal);
        // Any thread, any time. A signal that is not filterable reads as stop, log, pass.
        [[nodiscard]] SignalFilter Get(int signal) const;
        void Set(int signal, SignalFilter filter);
        // Resets every signal to its default, then applies text. A concurrent reader can see a mix of old and new.
        void Load(std::string_view text);

    private:
        std::array<std::atomic<uint8_t>, kMaxSignal + 1> mFilters;
    };
}
