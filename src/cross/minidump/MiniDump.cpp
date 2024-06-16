#include "MiniDump.h"
#include "Bridge.h"
#include "udmp-parser.h"
#include <QDebug>

#include "udmp-utils.h"

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

struct UserDumpParser : MiniDump::AbstractParser
{
    udmpparser::UserDumpParser mDmp;
    DumpMemoryProvider mMemory;

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
        return std::holds_alternative<udmpparser::Context64_t>(thread->Context);
    }

    std::vector<MiniDump::MemoryRegion> MemoryRegions() const override
    {
        std::vector<MiniDump::MemoryRegion> regions;
        const auto& mem = mDmp.GetMem();
        for(const auto& itr : mem)
        {
            regions.emplace_back();
            MiniDump::MemoryRegion& region = regions.back();
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
};

std::unique_ptr<MiniDump::AbstractParser> MiniDump::AbstractParser::Create(const uint8_t* begin, const uint8_t* end, std::string& error)
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
        auto parser = std::make_unique<UserDumpParser>();
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
        error = "PE files not yet supported!";
        return nullptr;
    }

    error = "Unsupported file format!";
    return nullptr;
}


Architecture* MiniDump::Architecture()
{
    return &gArchitecture;
}
