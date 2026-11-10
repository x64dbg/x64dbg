#include <ElfBug/process/ProcessList.h>
#include <ElfBug/process/ProcessArch.h>
#include <ElfBug/process/ProcFs.h>
#include <dirent.h>
#include <algorithm>

namespace ElfBug
{
    namespace
    {
        std::string readCmdline(const pid_t pid)
        {
            std::string cmdline = procfs::ReadFile(procfs::Path(pid, "cmdline"));
            while(!cmdline.empty() && cmdline.back() == '\0')
                cmdline.pop_back();
            std::replace(cmdline.begin(), cmdline.end(), '\0', ' ');
            return cmdline;
        }

        pid_t readStatusField(const pid_t pid, const std::string_view field)
        {
            const std::string status = procfs::ReadFile(procfs::Path(pid, "status"));
            return procfs::ParseNumber<pid_t>(procfs::FindValue(status, field)).value_or(0);
        }
    }

    pid_t TracerPid(const pid_t pid)
    {
        return readStatusField(pid, "TracerPid:");
    }

    pid_t ParentPid(const pid_t pid)
    {
        return readStatusField(pid, "PPid:");
    }

    pid_t ThreadGroupId(const pid_t pid)
    {
        return readStatusField(pid, "Tgid:");
    }

    bool ReadTaskList(const pid_t pid, std::vector<pid_t> & tids)
    {
        tids.clear();

        DIR* dir = opendir(procfs::Path(pid, "task").c_str());
        if(!dir)
            return false;

        while(const dirent* item = readdir(dir))
        {
            const pid_t tid = procfs::ParseNumber<pid_t>(item->d_name).value_or(0);
            if(tid > 0)
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
            const pid_t pid = procfs::ParseNumber<pid_t>(item->d_name).value_or(0);
            if(pid <= 0)
                continue;

            ProcessListEntry entry;
            entry.path = procfs::ReadLink(procfs::Path(pid, "exe"));
            if(entry.path.empty())
                continue;

            entry.pid = pid;
            entry.arch = DetectArchFromProcExe(pid);
            entry.traced = TracerPid(pid) != 0;
            entry.name = procfs::ReadLine(procfs::Path(pid, "comm"));
            entry.commandLine = readCmdline(pid);
            entries.push_back(std::move(entry));
        }

        closedir(dir);
        return entries;
    }
}
