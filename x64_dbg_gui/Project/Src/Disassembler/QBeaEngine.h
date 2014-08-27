#ifndef QBEAENGINE_H
#define QBEAENGINE_H

#include <QString>
#include "NewTypes.h"
#include "BeaTokenizer.h"

typedef struct _Instruction_t
{
    QString instStr;
    QByteArray dump;
    uint_t rva;
    int length;
    DISASM disasm;
    BeaTokenizer::BeaInstructionToken tokens;
} Instruction_t;

class QBeaEngine
{
public:
    explicit QBeaEngine();
    ulong DisassembleBack(byte_t* data, uint_t base, uint_t size, uint_t ip, int n);
    ulong DisassembleNext(byte_t* data, uint_t base, uint_t size, uint_t ip, int n);
    Instruction_t DisassembleAt(byte_t* data, uint_t size, uint_t instIndex, uint_t origBase, uint_t origInstRVA);

private:
    DISASM mDisasmStruct;
};

#endif // QBEAENGINE_H
