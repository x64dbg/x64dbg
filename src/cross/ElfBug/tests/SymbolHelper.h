#pragma once

#include <ElfBug/process/ProcFs.h>
#include <elf.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace ElfBug::test
{
    // Link-time address of `symbol` in `path`, via `nm`.
    inline std::optional<ptr> ResolveLinkAddress(const std::string & path, const std::string & symbol)
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

    // The aux vector value of `type` for pid, once pid's executable is `path`.
    inline std::optional<uint64_t> ReadExecAuxv(const pid_t pid, const std::string & path, const uint64_t type)
    {
        std::error_code exeError;
        std::error_code pathError;
        const auto exe = std::filesystem::canonical(procfs::Path(pid, "exe"), exeError);
        if(exeError || exe != std::filesystem::canonical(path, pathError) || pathError)
            return std::nullopt;
        const std::string auxv = procfs::ReadFile(procfs::Path(pid, "auxv"));
        for(size_t i = 0; i + 2 * sizeof(uint64_t) <= auxv.size(); i += 2 * sizeof(uint64_t))
        {
            uint64_t entry[2] = {};
            std::memcpy(entry, auxv.data() + i, sizeof(entry));
            if(entry[0] == type)
                return entry[1];
        }
        return std::nullopt;
    }

    inline std::optional<Elf64_Ehdr> ReadElfHeader(const std::string & path)
    {
        Elf64_Ehdr header{};
        std::ifstream file(path, std::ios::binary);
        if(!file.read(reinterpret_cast<char*>(&header), sizeof(header)))
            return std::nullopt;
        return header;
    }

    // Runtime minus link-time address for pid's executable `path`, from the aux vector, so extra mappings of the file don't matter.
    inline std::optional<ptr> GetExecLoadBias(const pid_t pid, const std::string & path)
    {
        const auto entry = ReadExecAuxv(pid, path, AT_ENTRY);
        const auto header = ReadElfHeader(path);
        if(!entry || !header)
            return std::nullopt;
        return *entry - header->e_entry;
    }

    // Where pid's executable `path` has its file offset 0 mapped.
    inline std::optional<ptr> GetExecBase(const pid_t pid, const std::string & path)
    {
        const auto phdr = ReadExecAuxv(pid, path, AT_PHDR);
        const auto header = ReadElfHeader(path);
        if(!phdr || !header)
            return std::nullopt;
        return *phdr - header->e_phoff;
    }

    inline std::optional<ptr> ResolveRuntimeAddress(const std::string & path, const pid_t pid, const std::string & symbol)
    {
        const auto linkAddress = ResolveLinkAddress(path, symbol);
        if(!linkAddress) return std::nullopt;
        const auto bias = GetExecLoadBias(pid, path);
        if(!bias) return std::nullopt;
        return *linkAddress + *bias;
    }

    struct SectionInfo
    {
        std::string name;
        ptr address = 0;
        ptr size = 0;
    };

    // The sections an ElfImage lists, by address: allocated, not empty, not TLS NOBITS and not lld's .relro_padding, via readelf -SW.
    inline std::vector<SectionInfo> ReadImageSections(const std::string & path)
    {
        std::vector<SectionInfo> sections;
        const std::string cmd = "readelf -SW '" + path + "' 2>/dev/null";
        FILE* pipe = popen(cmd.c_str(), "r");
        if(!pipe)
            return sections;

        char line[1024];
        while(std::fgets(line, sizeof(line), pipe))
        {
            const std::string_view text(line);
            const size_t open = text.find('[');
            const size_t close = text.find(']');
            if(open == std::string_view::npos || close == std::string_view::npos || close < open)
                continue;
            if(procfs::ParseNumber<unsigned>(text.substr(open + 1, close - open - 1)).value_or(0) == 0)
                continue;

            std::istringstream fields{std::string(text.substr(close + 1))};
            SectionInfo info;
            std::string type;
            std::string offset;
            std::string entrySize;
            std::string flags;
            fields >> info.name >> type >> std::hex >> info.address >> offset >> info.size >> entrySize >> flags;
            if(flags.find_first_not_of("0123456789") == std::string::npos)
                flags.clear();
            if(flags.find('A') == std::string::npos || info.size == 0 || (type == "NOBITS" && flags.find('T') != std::string::npos))
                continue;
            if(type == "NOBITS" && info.name == ".relro_padding")
                continue;
            sections.push_back(std::move(info));
        }
        pclose(pipe);
        std::sort(sections.begin(), sections.end(), [](const SectionInfo & a, const SectionInfo & b) { return a.address < b.address; });
        return sections;
    }
}
