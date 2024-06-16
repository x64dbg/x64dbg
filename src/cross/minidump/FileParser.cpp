#include <QDebug>

#include "FileParser.h"
#include "Bridge.h"
#include "StringUtil.h"

#include "udmp-parser.h"
#include "udmp-utils.h"

#include <linuxpe>

struct DumpMemoryProvider : MemoryProvider
{
    void setParser(udmpparser::UserDumpParser* parser)
    {
        mParser = parser;
    }

    bool read(duint addr, void* dest, duint size) override
    {
        if(mParser == nullptr)
            return false;

        auto block = mParser->GetMemBlock(addr);
        if(block == nullptr || block->State == MEM_FREE)
            return false;

        auto rva = addr - block->BaseAddress;

        // TODO: support page alignment zeroes
        if(rva + size >= block->DataSize)
            return false;

        memcpy(dest, block->Data + rva, size);
        return true;
    }

    bool getRange(duint addr, duint & base, duint & size) override
    {
        if(mParser == nullptr)
            return false;

        auto block = mParser->GetMemBlock(addr);
        if(block == nullptr || block->State == MEM_FREE)
            return false;

        base = block->BaseAddress;
        size = block->DataSize;
        return true;
    }

    bool isCodePtr(duint addr) override
    {
        auto block = mParser->GetMemBlock(addr);
        if(block == nullptr || block->State == MEM_FREE)
            return false;

        switch(block->Protect & 0xFF)
        {
        case PAGE_EXECUTE:
        case PAGE_EXECUTE_READ:
        case PAGE_EXECUTE_WRITECOPY:
        case PAGE_EXECUTE_READWRITE:
            return true;
        default:
            return false;
        }
    }

    bool isValidPtr(duint addr) override
    {
        auto block = mParser->GetMemBlock(addr);
        if(block == nullptr || block->State == MEM_FREE)
            return false;
        return true;
    }

private:
    udmpparser::UserDumpParser* mParser = nullptr;
};

struct GlobalArchitecture : Architecture
{
    bool disasm64() const override { return mDisasm64; }
    bool addr64() const override { return disasm64(); }

    void setDisasm64(bool disasm64)
    {
        mDisasm64 = disasm64;
    }

private:
    bool mDisasm64 = false;
} gArchitecture;

struct DmpFileParser : FileParser
{
    udmpparser::UserDumpParser mDmp;
    DumpMemoryProvider mMemory;
    uint64_t mEntryPoint = 0;

    bool disasm64() override
    {
        const auto & threads = mDmp.GetThreads();
        if(threads.empty())
        {
            qDebug() << "No threads in dump (this is unexpected)";
            return false;
        }

        auto threadId = mDmp.GetForegroundThreadId();
        const udmpparser::Thread_t* thread = nullptr;
        if(threadId.has_value())
        {
            thread = &mDmp.GetThreads().at(threadId.value());
        }
        else
        {
            thread = &mDmp.GetThreads().begin()->second;
        }
        bool disasm64 = false;
        std::visit([&](auto&& arg)
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr(std::is_same_v<T, udmpparser::Context64_t>)
            {
                disasm64 = true;
                mEntryPoint = arg.Rip;
            }
            else if constexpr(std::is_same_v<T, udmpparser::Context32_t>)
            {
                disasm64 = false;
                mEntryPoint = arg.Eip;
            }
        }, thread->Context);
        return disasm64;
    }

    std::vector<MemoryRegion> MemoryRegions() const override
    {
        std::vector<MemoryRegion> regions;
        const auto& mem = mDmp.GetMem();
        for(const auto& itr : mem)
        {
            regions.emplace_back();
            MemoryRegion& region = regions.back();
            const udmpparser::MemBlock_t & block = itr.second;
            region.BaseAddress = block.BaseAddress;
            region.RegionSize = block.RegionSize;
            region.State = StateToStringShort(block.State);
            if(block.State != MEM_FREE)
            {
                region.AllocationBase = block.AllocationBase;
                region.Protect = ProtectToStringShort(block.Protect);
                region.AllocationProtect = ProtectToStringShort(block.AllocationProtect);
                region.Type = TypeToStringShort(block.Type);
            }

            auto module = mDmp.GetModule(block.BaseAddress);
            if(module != nullptr)
            {
                // TODO: add module base here?
                region.Info = module->ModuleName;
            }
        }
        return regions;
    }

    uint64_t entryPoint() const override
    {
        return mEntryPoint;
    }
};

#define PAGE_SIZE 0x1000
#define ROUND_TO_PAGES(Size) (((uint64_t)(Size) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

struct PeFileParser : FileParser, MemoryProvider
{
    const uint8_t* mBegin = nullptr;
    const uint8_t* mEnd = nullptr;

    struct Section
    {
        std::string name;
        uint64_t addr = 0;
        uint64_t size = 0;
        uint32_t protect = 0;
        std::vector<uint8_t> data;
        const uint8_t* rawPtr = nullptr;
        uint64_t rawSize = 0;
    };

    bool mDisasm64 = false;
    uint64_t mImageBase = 0;
    uint64_t mSizeOfImage = 0;
    uint64_t mEntryPoint = 0;
    std::vector<Section> mSections;

    bool Parse(const uint8_t* begin, const uint8_t* end, std::string& error)
    {
        mBegin = begin;
        mEnd = end;

        auto pdh = (win::dos_header_t*)begin;
        if(pdh->e_magic != win::DOS_HDR_MAGIC)
        {
            error = "invalid DOS header magic";
            return false;
        }

        auto pfh = pdh->get_file_header();
        switch(pfh->machine)
        {
        case win::machine_id::amd64:
        {
            auto pnth = pdh->get_nt_headers<true>();
            if(pnth->signature != win::NT_HDR_MAGIC)
            {
                error = "invalid PE header magic";
                return false;
            }

            mDisasm64 = true;

            auto sectionAlignment = pnth->optional_header.section_alignment;
            auto fileAlignment = pnth->optional_header.file_alignment;
            if(sectionAlignment > PAGE_SIZE)
                sectionAlignment = PAGE_SIZE;
            mImageBase = pnth->optional_header.image_base;
            mSizeOfImage = ROUND_TO_PAGES(pnth->optional_header.size_image); // TODO: shouldn't this be regions?
            mEntryPoint = pnth->optional_header.entry_point;
            if(mEntryPoint > 0 || !pnth->file_header.characteristics.dll_file)
                mEntryPoint += mImageBase;

            auto sectionAlign = [sectionAlignment](duint value)
            {
                return (value + (sectionAlignment - 1)) & ~(sectionAlignment - 1);
            };

            auto fileAlign = [fileAlignment](duint value)
            {
                return (value + (fileAlignment - 1)) & ~(fileAlignment - 1);
            };

            // Align the sections
            for(uint32_t i = 0; i < pnth->file_header.num_sections; i++)
            {
                const auto& section = *pnth->get_section(i);

                std::string name;
                for(auto ch : section.name.short_name)
                {
                    if(ch == '\0')
                        break;
                    name.push_back(ch);
                }

                Section s;
                s.name = name;
                s.addr = mImageBase + sectionAlign(section.virtual_address);
                s.size = (section.virtual_size + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1);

                if(!section.characteristics.mem_read)
                {
                    s.protect = PAGE_NOACCESS;
                }
                else
                {
                    s.protect = section.characteristics.mem_write ? PAGE_READWRITE : PAGE_READONLY;
                    if(section.characteristics.mem_execute)
                    {
                        s.protect <<= 4;
                    }
                }

                // Extend the last section to SizeOfImage under the right circumstances
                if(i + 1 == pnth->file_header.num_sections && sectionAlignment < PAGE_SIZE)
                {
                    auto totalSize = s.addr + s.size;
                    totalSize -= mImageBase;
                    if(mSizeOfImage > totalSize)
                        s.size += mSizeOfImage - totalSize;
                }

                // TODO: check alignment
                s.rawPtr = begin + section.ptr_raw_data;
                s.rawSize = section.size_raw_data;
                if(s.rawSize > 0)
                {
                    if(s.rawPtr + s.rawSize > end)
                    {
                        s.name += " (invalid raw pointer)";
                        s.rawSize = 0;
                    }
                }

                qDebug() << "[section]" << "name:" << QString::fromStdString(name);
                qDebug() << "    " << "raw addr:" << ToHexString(section.ptr_raw_data) << "raw size:" << ToHexString(section.size_raw_data);
                qDebug() << "    " << "vaddr:" << ToHexString(section.virtual_address) << "vsize:" << ToHexString(section.virtual_size);
                qDebug() << "    ->" << "addr:" << ToHexString(s.addr) << "size:" << ToHexString(s.size) << "raw addr:" << ToHexString(s.rawPtr - begin) << "raw size:" << ToHexString(s.rawSize);

                mSections.push_back(s);
            }
        }
        break;

        case win::machine_id::i386:
        {
            auto pnth = pdh->get_nt_headers<false>();
            mDisasm64 = false;
            error = "32-bit executables not yet supported";
            return false;
        }
        break;

        default:
        {
            error = "unsupported architecture";
            return false;
        }
        }

        return true;
    }

    bool disasm64() override
    {
        return mDisasm64;
    }

    std::vector<MemoryRegion> MemoryRegions() const override
    {
        std::vector<MemoryRegion> regions;
        for(const auto& section : mSections)
        {
            regions.emplace_back();
            MemoryRegion& region = regions.back();
            region.BaseAddress = section.addr;
            region.RegionSize = section.size;
            region.State = StateToStringShort(MEM_COMMIT);
            region.AllocationBase = mImageBase;
            region.Protect = ProtectToStringShort(section.protect);
            region.AllocationProtect = ProtectToStringShort(PAGE_EXECUTE_READWRITE | PAGE_WRITECOMBINE);
            region.Type = TypeToStringShort(MEM_IMAGE);
            region.Info = section.name;
        }
        return regions;
    }

    uint64_t entryPoint() const override
    {
        return mEntryPoint;
    }

    bool read(duint addr, void* dest, duint size) override
    {
        for(const auto& section : mSections)
        {
            // TODO: support reading across sections
            if(addr >= section.addr && addr + size <= section.addr + section.size)
            {
                auto u8dest = (uint8_t*)dest;
                auto startOffset = addr - section.addr;
                for(size_t i = 0; i < size; i++)
                {
                    auto readOffset = startOffset + i;
                    if(readOffset < section.rawSize)
                    {
                        u8dest[i] = section.rawPtr[readOffset];
                    }
                    else
                    {
                        u8dest[i] = 0; // padding
                    }
                }

                return true;
            }
        }
        return false;
    }

    bool getRange(duint addr, duint & base, duint & size) override
    {
        for(const auto& section : mSections)
        {
            if(addr >= section.addr && addr < section.addr + section.size)
            {
                base = section.addr;
                size = section.size;
                return true;
            }
        }
        return false;
    }

    bool isCodePtr(duint addr) override
    {
        for(const auto& section : mSections)
        {
            if(addr >= section.addr && addr < section.addr + section.size)
            {
                return (section.protect & 0xF0) != 0;
            }
        }
        return false;
    }

    bool isValidPtr(duint addr) override
    {
        for(const auto& section : mSections)
        {
            if(addr >= section.addr && addr < section.addr + section.size)
            {
                return section.protect != PAGE_NOACCESS;
            }
        }
        return false;
    }
};

std::unique_ptr<FileParser> FileParser::Create(const uint8_t* begin, const uint8_t* end, std::string& error)
{
    // Invalidate the global memory provider (TODO: localize everything)
    DbgSetMemoryProvider(nullptr);

    auto size = end - begin;
    if(size < 4)
    {
        error = "File too small";
        return nullptr;
    }

    uint8_t magic[4];
    memcpy(&magic, begin, sizeof(magic));

    uint8_t mdmpMagic[4] = {'M', 'D', 'M', 'P'};
    if(memcmp(magic, mdmpMagic, sizeof(mdmpMagic)) == 0)
    {
        auto parser = std::make_unique<DmpFileParser>();
        if(!parser->mDmp.Parse(begin, end))
        {
            error = "Minidump parsing failed!";
            return nullptr;
        }

        gArchitecture.setDisasm64(parser->disasm64());
        parser->mMemory.setParser(&parser->mDmp);
        DbgSetMemoryProvider(&parser->mMemory);
        return parser;
    }

    uint8_t peMagic[2] = {'M', 'Z'};
    if(memcmp(magic, peMagic, sizeof(peMagic)) == 0)
    {
        auto parser = std::make_unique<PeFileParser>();
        if(!parser->Parse(begin, end, error))
        {
            error = "Failed to parse PE (" + error + ")";
            return nullptr;
        }

        gArchitecture.setDisasm64(parser->disasm64());
        DbgSetMemoryProvider(parser.get());
        return parser;
    }

    error = "Unsupported file format!";
    return nullptr;
}

Architecture* GlobalArchitecture()
{
    return &gArchitecture;
}
