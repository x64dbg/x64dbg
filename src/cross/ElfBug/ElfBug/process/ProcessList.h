#pragma once

#include <sys/types.h>
#include <string>
#include <vector>
#include <ElfBug/types/ElfBug.h>

namespace ElfBug
{
    struct ProcessListEntry
    {
        pid_t pid = 0;
        Arch arch = Arch::Unknown;
        bool traced = false;
        std::string name;
        std::string path;
        std::string commandLine;
    };

    // Excludes kernel threads and other users' processes: their exe link isn't readable.
    std::vector<ProcessListEntry> EnumProcesses();

    pid_t TracerPid(pid_t pid);

    pid_t ParentPid(pid_t pid);

    pid_t ThreadGroupId(pid_t pid);

    // Thread ids under /proc/<pid>/task. False means the process is gone.
    bool ReadTaskList(pid_t pid, std::vector<pid_t> & tids);
}
