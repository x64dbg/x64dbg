#ifndef QBEAENGINE_H
#define QBEAENGINE_H

#include <QString>
#include "Imports.h"
#include "BeaTokenizer.h"

typedef struct _Instruction_t
{
    QString instStr;
    QByteArray dump;
    duint rva;
    int length;
    DISASM disasm;
    BeaTokenizer::BeaInstructionToken tokens;
} Instruction_t;

class QBeaEngine
{
public:
    explicit QBeaEngine(int maxModuleSize);
    ulong DisassembleBack(byte_t* data, duint base, duint size, duint ip, int n);
    ulong DisassembleNext(byte_t* data, duint base, duint size, duint ip, int n);
    Instruction_t DisassembleAt(byte_t* data, duint size, duint instIndex, duint origBase, duint origInstRVA);

private:
    DISASM mDisasmStruct;
    int mMaxModuleSize;
};

#endif // QBEAENGINE_H
