#pragma once

#include <QThread>
#include "TraceFileReader.h"
#include "QZydis.h"

class TraceFileParser : public QThread
{
    Q_OBJECT
    friend class TraceFileReader;
    TraceFileParser(TraceFileReader* parent) : QThread(parent) {}
    static void readFileHeader(TraceFileReader* that);
    void run();
};

class TraceFilePage
{
public:
    TraceFilePage(TraceFileReader* parent, unsigned long long fileOffset, TRACEINDEX maxLength);
    TRACEINDEX Length() const;
    const REGDUMP & Registers(TRACEINDEX index) const;
    void OpCode(TRACEINDEX index, unsigned char* buffer, int* opcodeSize) const;
    const Instruction_t & Instruction(TRACEINDEX index, QZydis & mDisasm);
    DWORD ThreadId(TRACEINDEX index) const;
    int MemoryAccessCount(TRACEINDEX index) const;
    void MemoryAccessInfo(TRACEINDEX index, duint* address, duint* oldMemory, duint* newMemory, bool* isValid) const;

    FILETIME lastAccessed; //system user time

    void updateInstructions();

private:
    friend class TraceFileReader;
    TraceFileReader* mParent;
    std::vector<REGDUMP> mRegisters;
    QByteArray opcodes;
    std::vector<unsigned int> opcodeOffset;
    std::vector<unsigned char> opcodeSize;
    std::vector<Instruction_t> instructions;
    std::vector<unsigned int> memoryOperandOffset;
    std::vector<char> memoryFlags;
    std::vector<duint> memoryAddress;
    std::vector<duint> oldMemory;
    std::vector<duint> newMemory;
    std::vector<DWORD> threadId;
    TRACEINDEX length;
};
