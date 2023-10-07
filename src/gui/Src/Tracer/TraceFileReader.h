#pragma once

#include "Bridge.h"
#include <QFile>
#include <atomic>
#include "TraceFileDump.h"
#include "zydis_wrapper.h"

class TraceFileParser;
class TraceFilePage;
class QZydis;
struct Instruction_t;

#define MAX_MEMORY_OPERANDS 32

class TraceFileReader : public QObject
{
    Q_OBJECT
public:
    TraceFileReader(QObject* parent = NULL);
    ~TraceFileReader();
    bool Open(const QString & fileName);
    void Close();
    bool Delete();
    bool isError() const;
    //int Progress() const; // TODO: Trace view should start showing its first instructions as soon as they are loaded

    QString getIndexText(unsigned long long index) const;

    unsigned long long Length() const;

    REGDUMP Registers(unsigned long long index);
    void OpCode(unsigned long long index, unsigned char* buffer, int* opcodeSize);
    const Instruction_t & Instruction(unsigned long long index);
    // Get thread ID
    DWORD ThreadId(unsigned long long index);
    // Get number of memory accesses
    int MemoryAccessCount(unsigned long long index);
    // Get memory access information. Size of these buffers are MAX_MEMORY_OPERANDS.
    void MemoryAccessInfo(unsigned long long index, duint* address, duint* oldMemory, duint* newMemory, bool* isValid);
    // Get hash of EXE
    duint HashValue() const;
    const QString & ExePath() const;

    void purgeLastPage();

    void buildDumpTo(unsigned long long index);
    std::vector<unsigned long long> getReferences(duint startAddr, duint endAddr) const;
    TraceFileDump* getDump();

signals:
    void parseFinished();

public slots:
    void parseFinishedSlot();

private slots:
    void tokenizerUpdatedSlot();

private:
    typedef std::pair<unsigned long long, unsigned long long> Range;
    struct RangeCompare //from addrinfo.h
    {
        bool operator()(const Range & a, const Range & b) const //a before b?
        {
            return a.second < b.first;
        }
    };

    QFile traceFile;
    unsigned long long length;
    duint hashValue;
    QString EXEPath;
    std::vector<std::pair<unsigned long long, Range>> fileIndex; //index;<file offset;length>
    std::atomic<int> progress;
    bool error;
    TraceFilePage* lastAccessedPage;
    unsigned long long lastAccessedIndexOffset;
    friend class TraceFileParser;
    friend class TraceFilePage;

    TraceFileParser* parser;
    std::map<Range, TraceFilePage, RangeCompare> pages;
    TraceFilePage* getPage(unsigned long long index, unsigned long long* base);
    TraceFileDump dump;
    void buildDump(unsigned long long index);

    QZydis* mDisasm;
};

duint resolveZydisRegister(const REGDUMP & registers, ZydisRegister reg);
