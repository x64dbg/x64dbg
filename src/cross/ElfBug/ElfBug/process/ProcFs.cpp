#include <ElfBug/process/ProcFs.h>
#include <fcntl.h>
#include <unistd.h>
#include <algorithm>
#include <cerrno>

namespace ElfBug::procfs
{
    namespace
    {
        constexpr std::string_view kWhitespace = " \t\n";

        std::string_view takeToken(std::string_view & text)
        {
            text.remove_prefix(std::min(text.find_first_not_of(kWhitespace), text.size()));
            const size_t end = std::min(text.find_first_of(kWhitespace), text.size());
            const std::string_view token = text.substr(0, end);
            text.remove_prefix(end);
            return token;
        }

        std::string_view takePiece(std::string_view & text, const char separator)
        {
            const size_t end = std::min(text.find(separator), text.size());
            const std::string_view piece = text.substr(0, end);
            text.remove_prefix(std::min(end + 1, text.size()));
            return piece;
        }
    }

    std::string Path(const pid_t pid, const std::string_view name)
    {
        std::string path = "/proc/" + std::to_string(pid) + "/";
        path += name;
        return path;
    }

    std::string TaskPath(const pid_t pid, const pid_t tid, const std::string_view name)
    {
        std::string path = "/proc/" + std::to_string(pid) + "/task/" + std::to_string(tid) + "/";
        path += name;
        return path;
    }

    std::string ReadFile(const std::string & path)
    {
        std::string text;
        const int fd = open(path.c_str(), O_RDONLY | O_CLOEXEC);
        if(fd == -1)
            return text;

        // /proc files report a size of zero, so read until EOF.
        char chunk[4096];
        for(;;)
        {
            const ssize_t n = read(fd, chunk, sizeof(chunk));
            if(n == -1 && errno == EINTR)
                continue;
            if(n == -1)
                text.clear();
            if(n <= 0)
                break;
            text.append(chunk, static_cast<size_t>(n));
        }

        close(fd);
        return text;
    }

    std::string ReadLine(const std::string & path)
    {
        std::string text = ReadFile(path);
        if(const size_t newline = text.find('\n'); newline != std::string::npos)
            text.resize(newline);
        return text;
    }

    std::string_view Trim(const std::string_view text)
    {
        const size_t first = text.find_first_not_of(kWhitespace);
        if(first == std::string_view::npos)
            return {};
        return text.substr(first, text.find_last_not_of(kWhitespace) - first + 1);
    }

    std::vector<std::string_view> Split(std::string_view text, const char separator)
    {
        std::vector<std::string_view> pieces;
        while(!text.empty())
        {
            if(const std::string_view piece = takePiece(text, separator); !piece.empty())
                pieces.push_back(piece);
        }
        return pieces;
    }

    std::string_view FindValue(std::string_view text, const std::string_view key)
    {
        while(!text.empty())
        {
            const std::string_view line = takePiece(text, '\n');
            if(line.starts_with(key))
                return Trim(line.substr(key.size()));
        }
        return {};
    }

    // The kernel appends this to the path of an unlinked file.
    std::string_view StripDeletedSuffix(std::string_view path)
    {
        constexpr std::string_view deleted = " (deleted)";
        if(path.ends_with(deleted))
            path.remove_suffix(deleted.size());
        return path;
    }

    // start-end perms offset device inode [path]
    std::optional<MapsLine> ParseMapsLine(std::string_view line)
    {
        const std::string_view range = takeToken(line);
        const std::string_view perms = takeToken(line);
        const auto offset = ParseNumber<uint64_t>(takeToken(line), 16);
        const std::string_view device = takeToken(line);
        const std::string_view inode = takeToken(line);
        if(perms.size() != 4 || !offset || device.empty() || inode.empty())
            return std::nullopt;

        const size_t dash = range.find('-');
        if(dash == std::string_view::npos)
            return std::nullopt;
        const auto start = ParseNumber<uint64_t>(range.substr(0, dash), 16);
        const auto end = ParseNumber<uint64_t>(range.substr(dash + 1), 16);
        if(!start || !end)
            return std::nullopt;

        return MapsLine{*start, *end, *offset, perms, StripDeletedSuffix(Trim(line))};
    }

    // comm (field 2) can hold spaces and parentheses, so it runs to the last ')'.
    std::vector<std::string_view> StatFields(const std::string_view stat)
    {
        const size_t open = stat.find('(');
        const size_t close = stat.rfind(')');
        if(open == std::string_view::npos || close == std::string_view::npos || close < open)
            return {};

        std::vector<std::string_view> fields = {{}, Trim(stat.substr(0, open)), stat.substr(open + 1, close - open - 1)};
        for(const std::string_view field : Split(Trim(stat.substr(close + 1)), ' '))
            fields.push_back(field);
        return fields;
    }
}
