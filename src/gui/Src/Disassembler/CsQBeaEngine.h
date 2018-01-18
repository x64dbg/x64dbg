#ifndef CSQBEAENGINE_H
#define CSQBEAENGINE_H

#include <QString>
#include <vector>
#include "cs_capstone_gui.h"
#include "QBeaEngine.h" // for instruction_t

class EncodeMap;
class CodeFoldingHelper;

class CsQBeaEngine
{
public:
    explicit CsQBeaEngine(int maxModuleSize);
    ~CsQBeaEngine();
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
    CsCapstoneTokenizer _tokenizer;
    QHash<ENCODETYPE, DataInstructionInfo> dataInstMap;
    bool _bLongDataInst;
    EncodeMap* mEncodeMap;
    CodeFoldingHelper* mCodeFoldingManager;
    uint8_t reginfo[X86_REG_ENDING];
    uint8_t flaginfo[Capstone::FLAG_ENDING];
};

#endif // CSQBEAENGINE_H
