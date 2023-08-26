#pragma once

#include <QString>
#include <vector>
#include "ZydisTokenizer.h"

class EncodeMap;
class CodeFoldingHelper;

struct Instruction_t
{
    enum BranchType : uint8_t
    {
        None,
        Conditional,
        Unconditional,
        Call
    };

    duint rva = 0;
    duint branchDestination = 0;
    int length = 0;
    uint8_t vectorElementType[4];
    uint8_t prefixSize = 0;
    uint8_t opcodeSize = 0;
    uint8_t group1Size = 0;
    uint8_t group2Size = 0;
    uint8_t group3Size = 0;
    BranchType branchType = None;

    QString instStr;
    QByteArray dump;
    std::vector<std::pair<const char*, uint8_t>> regsReferenced;
    ZydisTokenizer::InstructionToken tokens;

    Instruction_t()
    {
        memset(vectorElementType, 0, sizeof(vectorElementType));
    }
};

class QZydis
{
public:
    QZydis(int maxModuleSize, Architecture* architecture);
    ~QZydis();
    ulong DisassembleBack(const uint8_t* data, duint base, duint size, duint ip, int n);
    ulong DisassembleNext(const uint8_t* data, duint base, duint size, duint ip, int n);
    Instruction_t DisassembleAt(const uint8_t* data, duint size, duint origBase, duint origInstRVA, bool datainstr = true);
    Instruction_t DecodeDataAt(const uint8_t* data, duint size, duint origBase, duint origInstRVA, ENCODETYPE type);
    void setCodeFoldingManager(CodeFoldingHelper* CodeFoldingManager);
    void UpdateConfig();
    void UpdateArchitecture();

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

    Architecture* mArchitecture = nullptr;
    ZydisTokenizer mTokenizer;
    QHash<ENCODETYPE, DataInstructionInfo> mDataInstMap;
    bool mLongDataInst = false;
    EncodeMap* mEncodeMap = nullptr;
    CodeFoldingHelper* mCodeFoldingManager = nullptr;
};

void formatOpcodeString(const Instruction_t & inst, RichTextPainter::List & list, std::vector<std::pair<size_t, bool>> & realBytes);
