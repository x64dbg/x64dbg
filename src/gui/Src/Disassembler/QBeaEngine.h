#ifndef QBEAENGINE_H
#define QBEAENGINE_H

#include <QString>
#include <vector>
#include "capstone_gui.h"

class EncodeMap;
class CodeFoldingHelper;

struct Instruction_t
{
    enum BranchType
    {
        None,
        Conditional,
        Unconditional,
        Call
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
    duint branchDestination;
    BranchType branchType;
    CapstoneTokenizer::InstructionToken tokens;
    std::vector<std::pair<const char*, uint8_t>> regsReferenced;
};

class QBeaEngine
{
public:
    explicit QBeaEngine(int maxModuleSize);
    ~QBeaEngine();
    ulong DisassembleBack(byte_t* data, duint base, duint size, duint ip, int n);
    ulong DisassembleNext(byte_t* data, duint base, duint size, duint ip, int n);
    Instruction_t DisassembleAt(byte_t* data, duint size, duint origBase, duint origInstRVA, bool datainstr = true);
    Instruction_t DecodeDataAt(byte_t* data, duint size, duint origBase, duint origInstRVA, ENCODETYPE type);
    void setCodeFoldingManager(CodeFoldingHelper* CodeFoldingManager);
    void UpdateConfig();

    EncodeMap* getEncodeMap()
    {
        return mEncodeMap;
    }

private:
    struct DataInstructionInfo
    {
        QString shortName;
        QString longName;
        QString cName;
    };

    void UpdateDataInstructionMap();
    CapstoneTokenizer _tokenizer;
    QHash<ENCODETYPE, DataInstructionInfo> dataInstMap;
    bool _bLongDataInst;
    EncodeMap* mEncodeMap;
    CodeFoldingHelper* mCodeFoldingManager;
    uint8_t reginfo[X86_REG_ENDING];
    uint8_t flaginfo[Capstone::FLAG_ENDING];
};

#endif // QBEAENGINE_H
