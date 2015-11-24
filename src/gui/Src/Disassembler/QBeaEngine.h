#ifndef QBEAENGINE_H
#define QBEAENGINE_H

#include <QString>
#include "Imports.h"
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
    duint rva;
    int length;
    //DISASM disasm;
    duint branchDestination;
    BranchType branchType;
    CapstoneTokenizer::InstructionToken tokens;
};

class QBeaEngine
{
public:
    explicit QBeaEngine(int maxModuleSize);
    ulong DisassembleBack(byte_t* data, duint base, duint size, duint ip, int n);
    ulong DisassembleNext(byte_t* data, duint base, duint size, duint ip, int n);
    Instruction_t DisassembleAt(byte_t* data, duint size, duint instIndex, duint origBase, duint origInstRVA);
    void UpdateConfig();

private:
    CapstoneTokenizer _tokenizer;
};

#endif // QBEAENGINE_H
