#pragma once

#include <ElfBug/process/ProcFs.h>
#include <cstdio>
#include <optional>
#include <sstream>
#include <string>

namespace ElfBug::test
{
    // ELF file offset of `symbol` in `path`, via `nm`.
    inline std::optional<ptr> ResolveFileOffset(const std::string & path, const std::string & symbol)
    {
        const std::string cmd = "nm --defined-only '" + path + "' 2>/dev/null";
        FILE* pipe = popen(cmd.c_str(), "r");
        if(!pipe) return std::nullopt;

        char line[512];
        std::optional<ptr> found;
        while(std::fgets(line, sizeof(line), pipe))
        {
            std::istringstream iss(line);
            unsigned long long addr = 0;
            char type = 0;
            std::string name;
            if((iss >> std::hex >> addr >> type >> name) && symbol == name)
            {
                found = static_cast<ptr>(addr);
                break;
            }
        }
        pclose(pipe);
        return found;
    }

    // Runtime-minus-file offset for `path`'s executable mapping in pid's /proc/maps.
    inline std::optional<ptr> GetExecLoadBias(const pid_t pid, const std::string & path)
    {
        const std::string maps = procfs::ReadFile(procfs::Path(pid, "maps"));
        for(const std::string_view line : procfs::Split(maps, '\n'))
        {
            const auto entry = procfs::ParseMapsLine(line);
            if(entry && entry->perms[2] == 'x' && entry->path == path)
                return entry->start - entry->offset;
        }
        return std::nullopt;
    }

    inline std::optional<ptr> ResolveRuntimeAddress(const std::string & path, const pid_t pid, const std::string & symbol)
    {
        const auto fileOffset = ResolveFileOffset(path, symbol);
        if(!fileOffset) return std::nullopt;
        const auto bias = GetExecLoadBias(pid, path);
        if(!bias) return std::nullopt;
        return *fileOffset + *bias;
    }
}
