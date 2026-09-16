#include <ElfBug/process/ProcessList.h>
#include <ElfBug/process/ProcessArch.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace ElfBug
{
    namespace
    {
        ssize_t readAll(const char* path, char* buffer, const size_t size)
        {
            const int fd = open(path, O_RDONLY | O_CLOEXEC);
            if(fd == -1)
                return -1;

            size_t total = 0;
            while(total + 1 < size)
            {
                const ssize_t n = read(fd, buffer + total, size - total - 1);
                if(n == -1)
                {
                    if(errno == EINTR)
                        continue;
                    close(fd);
                    return -1;
                }
                if(n == 0)
                    break;
                total += static_cast<size_t>(n);
            }

            close(fd);
            buffer[total] = '\0';
            return static_cast<ssize_t>(total);
        }

        // The kernel appends " (deleted)" when the executable has been unlinked, and
        // truncates silently at the buffer size rather than reporting it.
        std::string readLink(const char* path)
        {
            std::string result;
            for(size_t size = 256; size <= 65536; size *= 2)
            {
                result.resize(size);
                const ssize_t n = readlink(path, result.data(), size);
                if(n <= 0)
                    return {};
                if(static_cast<size_t>(n) < size)
                {
                    result.resize(static_cast<size_t>(n));
                    constexpr char deleted[] = " (deleted)";
                    constexpr size_t deletedLen = sizeof(deleted) - 1;
                    if(result.size() > deletedLen &&
                            result.compare(result.size() - deletedLen, deletedLen, deleted) == 0)
                        result.resize(result.size() - deletedLen);
                    return result;
                }
            }
            return {};
        }

        std::string readComm(const pid_t pid)
        {
            char path[64];
            snprintf(path, sizeof(path), "/proc/%d/comm", pid);
            char buffer[256];
            const ssize_t n = readAll(path, buffer, sizeof(buffer));
            if(n <= 0)
                return {};
            std::string comm(buffer, static_cast<size_t>(n));
            while(!comm.empty() && comm.back() == '\n')
                comm.pop_back();
            return comm;
        }

        std::string readCmdline(const pid_t pid)
        {
            char path[64];
            snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
            char buffer[4096];
            const ssize_t n = readAll(path, buffer, sizeof(buffer));
            if(n <= 0)
                return {};
            std::string raw(buffer, static_cast<size_t>(n));
            while(!raw.empty() && raw.back() == '\0')
                raw.pop_back();
            for(auto & c : raw)
            {
                if(c == '\0')
                    c = ' ';
            }
            return raw;
        }

        bool readStatus(const pid_t pid, pid_t* tracer, pid_t* parent, pid_t* group)
        {
            char path[64];
            snprintf(path, sizeof(path), "/proc/%d/status", pid);
            char buffer[4096];
            if(readAll(path, buffer, sizeof(buffer)) <= 0)
                return false;

            for(const char* line = buffer; line && *line;)
            {
                if(tracer && strncmp(line, "TracerPid:", 10) == 0)
                    *tracer = static_cast<pid_t>(strtol(line + 10, nullptr, 10));
                else if(parent && strncmp(line, "PPid:", 5) == 0)
                    *parent = static_cast<pid_t>(strtol(line + 5, nullptr, 10));
                else if(group && strncmp(line, "Tgid:", 5) == 0)
                    *group = static_cast<pid_t>(strtol(line + 5, nullptr, 10));

                const char* next = strchr(line, '\n');
                line = next ? next + 1 : nullptr;
            }
            return true;
        }

        bool parsePid(const char* name, pid_t & out)
        {
            char* end = nullptr;
            const long value = strtol(name, &end, 10);
            if(end == name || *end != '\0' || value <= 0)
                return false;
            out = static_cast<pid_t>(value);
            return true;
        }
    }

    pid_t TracerPid(const pid_t pid)
    {
        pid_t tracer = 0;
        readStatus(pid, &tracer, nullptr, nullptr);
        return tracer;
    }

    pid_t ParentPid(const pid_t pid)
    {
        pid_t parent = 0;
        readStatus(pid, nullptr, &parent, nullptr);
        return parent;
    }

    pid_t ThreadGroupId(const pid_t pid)
    {
        pid_t group = 0;
        readStatus(pid, nullptr, nullptr, &group);
        return group;
    }

    bool ReadTaskList(const pid_t pid, std::vector<pid_t> & tids)
    {
        tids.clear();

        char path[64];
        snprintf(path, sizeof(path), "/proc/%d/task", pid);
        DIR* dir = opendir(path);
        if(!dir)
            return false;

        while(const dirent* item = readdir(dir))
        {
            pid_t tid = 0;
            if(parsePid(item->d_name, tid))
                tids.push_back(tid);
        }

        closedir(dir);
        return true;
    }

    std::vector<ProcessListEntry> EnumProcesses()
    {
        std::vector<ProcessListEntry> entries;

        DIR* dir = opendir("/proc");
        if(!dir)
            return entries;

        while(const dirent* item = readdir(dir))
        {
            pid_t pid = 0;
            if(!parsePid(item->d_name, pid))
                continue;

            char path[64];
            snprintf(path, sizeof(path), "/proc/%d/exe", pid);

            ProcessListEntry entry;
            entry.path = readLink(path);
            if(entry.path.empty())
                continue;

            entry.pid = pid;
            entry.arch = detectArchFromProcExe(pid);
            entry.traced = TracerPid(pid) != 0;
            entry.name = readComm(pid);
            entry.commandLine = readCmdline(pid);
            entries.push_back(std::move(entry));
        }

        closedir(dir);
        return entries;
    }
}
