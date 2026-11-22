#include <ElfBug/core/SignalFilters.h>
#include <ElfBug/process/ProcFs.h>
#include <csignal>
#include <optional>

namespace ElfBug
{
    namespace
    {
        constexpr uint8_t kStopBit = 1;
        constexpr uint8_t kLogBit = 2;
        constexpr uint8_t kPassBit = 4;

        // Reserved by the C library for its own use: glibc takes 32 and 33, musl 32 to 34.
        constexpr int kFirstLibcSignal = 32;
        constexpr int kLastLibcSignal = 34;

        struct FilterEntry
        {
            int first = 0;
            int last = 0;
            SignalFilter filter;
        };

        uint8_t pack(const SignalFilter filter)
        {
            return (filter.stop ? kStopBit : 0) | (filter.log ? kLogBit : 0) | (filter.pass ? kPassBit : 0);
        }

        SignalFilter unpack(const uint8_t bits)
        {
            return {(bits & kStopBit) != 0, (bits & kLogBit) != 0, (bits & kPassBit) != 0};
        }

        bool contains(const std::string_view entry, const std::string_view token)
        {
            return entry.find(token) != std::string_view::npos;
        }

        std::optional<FilterEntry> parseEntry(const std::string_view entry)
        {
            const std::string_view range = entry.substr(0, entry.find(':'));
            const size_t dash = range.find('-');
            if(dash == std::string_view::npos)
                return std::nullopt;
            const auto first = procfs::ParseNumber<int>(range.substr(0, dash), 16);
            const auto last = procfs::ParseNumber<int>(range.substr(dash + 1), 16);
            if(!first || !last || *first < 1 || *first > *last || *last > SignalFilters::kMaxSignal)
                return std::nullopt;
            if(!contains(entry, "debugger") && !contains(entry, "debuggee"))
                return std::nullopt;
            SignalFilter filter;
            filter.stop = contains(entry, "first") || contains(entry, "second");
            filter.log = !contains(entry, "nolog");
            filter.pass = !contains(entry, "debugger");
            return FilterEntry{*first, *last, filter};
        }
    }

    SignalFilters::SignalFilters()
    {
        Load({});
    }

    bool SignalFilters::Filterable(const int signal)
    {
        return signal >= 1 && signal <= kMaxSignal && signal != SIGKILL && signal != SIGSTOP;
    }

    SignalFilter SignalFilters::Default(const int signal)
    {
        const SignalFilter quiet{false, false, true};
        if(signal >= kFirstLibcSignal && signal <= kLastLibcSignal)
            return quiet;
        switch(signal)
        {
        case SIGCHLD:
        case SIGALRM:
        case SIGVTALRM:
        case SIGPROF:
        case SIGURG:
        case SIGIO:
        case SIGWINCH:
            return quiet;
        default:
            return {};
        }
    }

    SignalFilter SignalFilters::Get(const int signal) const
    {
        if(!Filterable(signal))
            return {};
        return unpack(mFilters[signal].load(std::memory_order_relaxed));
    }

    void SignalFilters::Set(const int signal, const SignalFilter filter)
    {
        if(Filterable(signal))
            mFilters[signal].store(pack(filter), std::memory_order_relaxed);
    }

    void SignalFilters::Load(const std::string_view text)
    {
        std::array<SignalFilter, kMaxSignal + 1> table;
        for(int signal = 1; signal <= kMaxSignal; signal++)
            table[signal] = Default(signal);
        for(const std::string_view entry : procfs::Split(text, ','))
        {
            const auto parsed = parseEntry(entry);
            if(!parsed)
                continue;
            for(int signal = parsed->first; signal <= parsed->last; signal++)
                table[signal] = parsed->filter;
        }
        for(int signal = 1; signal <= kMaxSignal; signal++)
            Set(signal, table[signal]);
    }
}
