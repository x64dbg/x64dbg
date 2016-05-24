#ifndef TRACERECORD_H
#define TRACERECORD_H

class TraceRecordManager
{
public:
    enum{
        InstructionBody,
        InstructionHeading,
        InstructionTailing,
        InstructionOverlapped, // The byte was executed with differing instruction base addresses
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
    } TraceRecordByteType;

    /***************************************************************
     * Trace record data layout
     * TraceRecordNonoe: disable trace record
     * TraceRecordBitExec: single-bit, executed.
     * TraceRecordByteWithExecTypeAndCounter: 8-bit, YYXXXXXX YY:=TraceRecordByteType_2bit, XXXXXX:=Hit count(6bit)
     * TraceRecordWordWithExecTypeAndCounter: 16-bit, YYXXXXXX XXXXXXXX YY:=TraceRecordByteType_2bit, XX:=Hit count(14bit)
     * Other: reserved for future expanding
     **************************************************************/
    enum{
        TraceRecordNone,
        TraceRecordBitExec,
        TraceRecordByteWithExecTypeAndCounter,
        TraceRecordWordWithExecTypeAndCounter
    } TraceRecordType;

    TraceRecordManager();
    ~TraceRecordManager();

    bool setTraceRecordType(duint pageAddress, TraceRecordType type);
    TraceRecordType getTraceRecordType(duint pageAddress);

    void TraceExecute(duint address, unsigned char size);
    //void TraceAccess(duint address, unsigned char size, TraceRecordByteType accessType);

    bool isTraced(duint address);
    unsigned int getHitCount(duint address);
    TraceRecordByteType getByteType(duint address);

    bool loadFromDatabase();
    bool saveToDatabase();

private:
    enum{
        InstructionBody = 0,
        InstructionHeading = 1,
        InstructionTailing = 2,
        InstructionOverlapped = 3
    } TraceRecordByteType_2bit;

    struct{
        void* rawPtr;
        TraceRecordType dataType;
    } TraceRecordPage;

    //Key := page base, value := trace record raw data
    QMap<duint, TraceRecordPage> TraceRecord;
};

#endif // TRACERECORD_H
