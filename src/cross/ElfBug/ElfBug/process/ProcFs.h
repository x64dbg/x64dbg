#pragma once

#include <sys/types.h>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ElfBug::procfs
{
    constexpr size_t kStatState = 3;
    constexpr size_t kStatUtime = 14;
    constexpr size_t kStatStime = 15;
    constexpr size_t kStatNice = 19;
    constexpr size_t kStatStartTime = 22;
    constexpr size_t kStatRtPriority = 40;
    constexpr size_t kStatPolicy = 41;

    struct MapsLine
    {
        uint64_t start = 0;
        uint64_t end = 0;
        uint64_t offset = 0;
        std::string_view perms;
        std::string_view path;
    };

    std::string Path(pid_t pid, std::string_view name);

    std::string TaskPath(pid_t pid, pid_t tid, std::string_view name);

    // Empty when the file can't be read.
    std::string ReadFile(const std::string & path);

    std::string ReadLine(const std::string & path);

    std::string_view Trim(std::string_view text);

    // Empty pieces are dropped.
    std::vector<std::string_view> Split(std::string_view text, char separator);

    // The rest of the first line that starts with key, trimmed.
    std::string_view FindValue(std::string_view text, std::string_view key);

    std::string_view StripDeletedSuffix(std::string_view path);

    std::optional<MapsLine> ParseMapsLine(std::string_view line);

    // Numbered from 1 like the man page, so fields[kStatUtime] is utime. Empty if malformed.
    std::vector<std::string_view> StatFields(std::string_view stat);

    template<typename T>
    std::optional<T> ParseNumber(std::string_view text, const int base = 10)
    {
        text = Trim(text);
        T value{};
        const char* end = text.data() + text.size();
        const auto result = std::from_chars(text.data(), end, value, base);
        if(result.ec != std::errc() || result.ptr != end)
            return std::nullopt;
        return value;
    }
}
