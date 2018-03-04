#ifndef TRACERECORD_H
#define TRACERECORD_H
#include "_global.h"
#include "_dbgfunctions.h"
#include "debugger.h"
#include "jansson/jansson_x64dbg.h"
#include <zydis_wrapper.h>

class Capstone;

class TraceRecordManager
{
public:
    enum TraceRecordByteType
    {
        InstructionBody = 0,
        InstructionHeading = 1,
        InstructionTailing = 2,
        InstructionOverlapped = 3, // The byte was executed with differing instruction base addresses
        DataByte,  // This and the following is not implemented yet.
        DataWord,
        DataDWord,
        DataQWord,
        DataFloat,
        DataDouble,
        DataLongDouble,
        DataXMM,
        DataYMM,
        DataMMX,
        DataMixed, //the byte is accessed in multiple ways
        InstructionDataMixed //the byte is both executed and written
    };

    /***************************************************************
     * Trace record data layout
     * TraceRecordNonoe: disable trace record
     * TraceRecordBitExec: single-bit, executed.
     * TraceRecordByteWithExecTypeAndCounter: 8-bit, YYXXXXXX YY:=TraceRecordByteType_2bit, XXXXXX:=Hit count(6bit)
     * TraceRecordWordWithExecTypeAndCounter: 16-bit, YYXXXXXX XXXXXXXX YY:=TraceRecordByteType_2bit, XX:=Hit count(14bit)
     * Other: reserved for future expanding
     **************************************************************/
    enum TraceRecordType
    {
        TraceRecordNone,
        TraceRecordBitExec,
        TraceRecordByteWithExecTypeAndCounter,
        TraceRecordWordWithExecTypeAndCounter
    };

    TraceRecordManager();
    ~TraceRecordManager();
    void clear();

    bool setTraceRecordType(duint pageAddress, TraceRecordType type);
    TraceRecordType getTraceRecordType(duint pageAddress);

    void TraceExecute(duint address, duint size);
    //void TraceAccess(duint address, unsigned char size, TraceRecordByteType accessType);
    void TraceExecuteRecord(const Zydis & newInstruction);

    unsigned int getHitCount(duint address);
    TraceRecordByteType getByteType(duint address);
    void increaseInstructionCounter();

    bool isRunTraceEnabled();
    bool enableRunTrace(bool enabled, const char* fileName);

    void saveToDb(JSON root);
    void loadFromDb(JSON root);
private:
    enum TraceRecordByteType_2bit
    {
        _InstructionBody = 0,
        _InstructionHeading = 1,
        _InstructionTailing = 2,
        _InstructionOverlapped = 3
    };

    struct TraceRecordPage
    {
        void* rawPtr;
        duint rva;
        TraceRecordType dataType;
        unsigned int moduleIndex;
    };

    typedef union _REGDUMPWORD
    {
        REGDUMP registers;
        // 172 qwords on x64, 216 dwords on x86. Almost no space left for AVX512
        // strip off lastStatus and 128 bytes of lastError.name member.
        duint regword[(FIELD_OFFSET(REGDUMP, lastError) + sizeof(DWORD)) / sizeof(duint)];
    } REGDUMPWORD;

    //Key := page base, value := trace record raw data
    std::unordered_map<duint, TraceRecordPage> TraceRecord;
    std::vector<std::string> ModuleNames;
    unsigned int getModuleIndex(const String & moduleName);
    unsigned int instructionCounter;

    bool rtEnabled;
    bool rtPrevInstAvailable;
    HANDLE rtFile;

    REGDUMPWORD rtOldContext;
    bool rtOldContextChanged[(FIELD_OFFSET(REGDUMP, lastError) + sizeof(DWORD)) / sizeof(duint)];
    DWORD rtOldThreadId;
    bool rtNeedThreadId;
    duint rtOldMemory[32];
    duint rtOldMemoryAddress[32];
    char rtOldOpcode[16];
    unsigned int rtRecordedInstructions;
    unsigned char rtOldOpcodeSize;
    unsigned char rtOldMemoryArrayCount;
};

extern TraceRecordManager TraceRecord;
void _dbg_dbgtraceexecute(duint CIP);

//exported to bridge
unsigned int _dbg_dbggetTraceRecordHitCount(duint address);
TRACERECORDBYTETYPE _dbg_dbggetTraceRecordByteType(duint address);
bool _dbg_dbgsetTraceRecordType(duint pageAddress, TRACERECORDTYPE type);
TRACERECORDTYPE _dbg_dbggetTraceRecordType(duint pageAddress);
bool _dbg_dbgenableRunTrace(bool enabled, const char* fileName);
bool _dbg_dbgisRunTraceEnabled();

#endif // TRACERECORD_H
