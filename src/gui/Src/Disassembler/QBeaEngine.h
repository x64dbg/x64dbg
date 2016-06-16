#ifndef QBEAENGINE_H
#define QBEAENGINE_H

#include <QString>
#include "Imports.h"
#include "capstone_gui.h"
#include "EncodeMap.h"

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
    ~QBeaEngine();
    ulong DisassembleBack(byte_t* data, duint base, duint size, duint ip, int n);
    ulong DisassembleNext(byte_t* data, duint base, duint size, duint ip, int n);
    Instruction_t DisassembleAt(byte_t* data, duint size, duint instIndex, duint origBase, duint origInstRVA);
    Instruction_t DecodeDataAt(byte_t* data, duint size, duint instIndex, duint origBase, duint origInstRVA, ENCODETYPE type);
    void UpdateConfig();
    EncodeMap* getEncodeMap() { return mEncodeMap; }
    void setCIP(duint cip) { mCIP = cip; }


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
    duint mCIP;


};

#endif // QBEAENGINE_H
