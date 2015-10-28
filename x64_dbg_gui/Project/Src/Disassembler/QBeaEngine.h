#ifndef QBEAENGINE_H
#define QBEAENGINE_H

#include <QString>
#include "NewTypes.h"
#include "BeaTokenizer.h"
#include "capstone_gui.h"

struct Instruction_t
{
    enum BranchType
    {
        None,
        Conditional,
        Unconditional
    };

    Instruction_t()
        : rva(0),
          length(0),
          branchDestination(0),
          branchType(None)
    {
    }

    QString instStr;
    QByteArray dump;
    uint_t rva;
    int length;
    //DISASM disasm;
    uint_t branchDestination;
    BranchType branchType;
    BeaTokenizer::BeaInstructionToken tokens;
};

class QBeaEngine
{
public:
    explicit QBeaEngine(int maxModuleSize);
    ulong DisassembleBack(byte_t* data, uint_t base, uint_t size, uint_t ip, int n);
    ulong DisassembleNext(byte_t* data, uint_t base, uint_t size, uint_t ip, int n);
    Instruction_t DisassembleAt(byte_t* data, uint_t size, uint_t instIndex, uint_t origBase, uint_t origInstRVA);
    void UpdateConfig();

private:
    int mMaxModuleSize;
    CapstoneTokenizer _tokenizer;
};

#endif // QBEAENGINE_H
