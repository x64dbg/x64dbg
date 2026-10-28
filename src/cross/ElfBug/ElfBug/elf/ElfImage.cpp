#include <ElfBug/elf/ElfImage.h>
#include <elf.h>
#include <algorithm>
#include <cstring>

namespace ElfBug
{
    namespace
    {
        constexpr size_t kMaxProgramHeaders = 1024;
        constexpr uint64_t kMaxSectionTable = 4 * 1024 * 1024;
        constexpr uint64_t kMaxNameTable = 1024 * 1024;

        std::vector<ElfSection> readSections(const ElfReader & read, const Elf64_Ehdr & header)
        {
            if(header.e_shoff == 0 || header.e_shentsize != sizeof(Elf64_Shdr))
                return {};

            uint64_t count = header.e_shnum;
            uint64_t namesIndex = header.e_shstrndx;
            if(count == 0 || namesIndex == SHN_XINDEX)
            {
                Elf64_Shdr first{};
                if(!read(header.e_shoff, &first, sizeof(first)))
                    return {};
                if(count == 0)
                    count = first.sh_size;
                if(namesIndex == SHN_XINDEX)
                    namesIndex = first.sh_link;
            }
            if(count == 0 || count > kMaxSectionTable / sizeof(Elf64_Shdr) || namesIndex >= count)
                return {};

            std::vector<Elf64_Shdr> headers(count);
            if(!read(header.e_shoff, headers.data(), headers.size() * sizeof(Elf64_Shdr)))
                return {};

            const Elf64_Shdr & names = headers[namesIndex];
            if(names.sh_type != SHT_STRTAB || names.sh_size == 0 || names.sh_size > kMaxNameTable)
                return {};
            std::string table(names.sh_size, '\0');
            if(!read(names.sh_offset, table.data(), table.size()) || table.back() != '\0')
                return {};

            std::vector<ElfSection> sections;
            for(const auto & section : headers)
            {
                if(!(section.sh_flags & SHF_ALLOC) || section.sh_size == 0)
                    continue;
                // .tbss takes no address space in the image.
                if((section.sh_flags & SHF_TLS) && section.sh_type == SHT_NOBITS)
                    continue;
                if(section.sh_name >= table.size())
                    return {};
                const char* name = table.c_str() + section.sh_name;
                // lld's .relro_padding only pads RELRO out to a page.
                if(section.sh_type == SHT_NOBITS && std::strcmp(name, ".relro_padding") == 0)
                    continue;
                sections.push_back({name, section.sh_addr, section.sh_size});
            }
            std::sort(sections.begin(), sections.end(), [](const ElfSection & a, const ElfSection & b)
            {
                return a.addr < b.addr;
            });
            return sections;
        }
    }

    std::optional<ElfImage> ElfImage::Parse(const ElfReader & read, const Parts parts)
    {
        Elf64_Ehdr header{};
        if(!read(0, &header, sizeof(header)))
            return std::nullopt;
        if(std::memcmp(header.e_ident, ELFMAG, SELFMAG) != 0 || header.e_ident[EI_CLASS] != ELFCLASS64 ||
                header.e_ident[EI_DATA] != ELFDATA2LSB || header.e_machine != EM_X86_64 ||
                (header.e_type != ET_EXEC && header.e_type != ET_DYN))
            return std::nullopt;
        if(header.e_phentsize != sizeof(Elf64_Phdr) || header.e_phnum == 0 || header.e_phnum > kMaxProgramHeaders)
            return std::nullopt;

        std::vector<Elf64_Phdr> programs(header.e_phnum);
        if(!read(header.e_phoff, programs.data(), programs.size() * sizeof(Elf64_Phdr)))
            return std::nullopt;

        ElfImage image;
        const auto* headerBytes = reinterpret_cast<const std::byte*>(&header);
        const auto* programBytes = reinterpret_cast<const std::byte*>(programs.data());
        image.mHeaderBytes.assign(headerBytes, headerBytes + sizeof(header));
        image.mHeaderBytes.insert(image.mHeaderBytes.end(), programBytes, programBytes + programs.size() * sizeof(Elf64_Phdr));
        for(const auto & program : programs)
        {
            if(program.p_type != PT_LOAD)
                continue;
            const ElfSegment segment{program.p_vaddr, program.p_memsz, program.p_offset, program.p_filesz, program.p_flags};
            if(segment.End() < segment.vaddr)
                return std::nullopt;
            image.mSegments.push_back(segment);
        }
        if(parts == Parts::HeadersAndSections)
            image.mSections = readSections(read, header);
        return image;
    }

    std::optional<uint64_t> ElfImage::LoadBias(const uint64_t mappingStart, const uint64_t mappingOffset) const
    {
        for(const auto & segment : mSegments)
        {
            if(PageFloor(segment.offset) == mappingOffset)
                return mappingStart - PageFloor(segment.vaddr);
        }
        return std::nullopt;
    }

    uint64_t ElfImage::End() const
    {
        uint64_t end = 0;
        for(const auto & segment : mSegments)
            end = std::max(end, segment.End());
        return end;
    }
}
