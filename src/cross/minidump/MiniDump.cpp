#include "MiniDump.h"
#include "Bridge.h"
#include <QDebug>

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
} gDumpMemory;

struct DumpArchitecture : Architecture
{
    bool disasm64() const override { return mDisasm64; }
    bool addr64() const override { return disasm64(); }

    void setParser(udmpparser::UserDumpParser* parser)
    {
        const auto & threads = parser->GetThreads();
        if(threads.empty())
        {
            qDebug() << "No threads in dump (this is unexpected)";
            mDisasm64 = false;
            return;
        }

        auto threadId = parser->GetForegroundThreadId();
        const udmpparser::Thread_t* thread = nullptr;
        if(threadId.has_value())
        {
            thread = &parser->GetThreads().at(threadId.value());
        }
        else
        {
            thread = &parser->GetThreads().begin()->second;
        }
        mDisasm64 = std::holds_alternative<udmpparser::Context64_t>(thread->Context);
    }

private:
    bool mDisasm64 = false;
} gDumpArchitecture;

void MiniDump::Load(udmpparser::UserDumpParser* parser)
{
    gDumpArchitecture.setParser(parser);
    gDumpMemory.setParser(parser);
    DbgSetMemoryProvider(&gDumpMemory);
}

Architecture* MiniDump::Architecture()
{
    return &gDumpArchitecture;
}
