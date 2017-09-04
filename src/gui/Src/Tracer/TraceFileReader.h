#ifndef TRACEFILEREADER_H
#define TRACEFILEREADER_H

#include "Bridge.h"
#include <QFile>
#include <QThread>
#include <atomic>

class TraceFileReader : public QObject
{
    Q_OBJECT
public:
    TraceFileReader(QObject* parent = NULL);
    bool Open(const QString & fileName);
    void Close();
    bool isError();
    int Progress();

    unsigned long long Length();

    REGDUMP Registers(unsigned long long index);
    void OpCode(unsigned long long index, unsigned char* buffer);
    DWORD ThreadId(unsigned long long index);
    int MemoryAccessCount(unsigned long long index);
    void MemoryAccessInfo(unsigned long long index, duint* address, duint* oldMemory, duint* newMemory, bool* isValid);

private slots:

    void parseFinished();

private:
    struct RangeCompare //from addrinfo.h
    {
        bool operator()(const Range & a, const Range & b) const //a before b?
        {
            return a.second < b.first;
        }
    };
    typedef std::pair<unsigned long long, unsigned long long> Range;

    QFile traceFile;
    unsigned long long length;
    std::vector<unsigned long long, Range> fileIndex; //index->file offset, index;<file offset;maxindex>
    std::atomic<int> progress;
    bool error;
    TraceFilePage* lastAccessedPage;
    unsigned long long lastAccessedIndexOffset;

    class TraceFileParser : public QThread
    {
        Q_OBJECT
        TraceFileParser(TraceFileReader* parent) : QThread(parent) {}
        void run();
    };

    class TraceFilePage
    {
    public:
        TraceFilePage(TraceFileReader* parent, unsigned long long fileOffset, unsigned long long maxLength);
        unsigned long long Length() const;
        REGDUMP Registers(unsigned long long index) const;
        void OpCode(unsigned long long index, unsigned char* buffer) const;
        DWORD ThreadId(unsigned long long index) const;
        int MemoryAccessCount(unsigned long long index) const;
        void MemoryAccessInfo(unsigned long long index, duint* address, duint* oldMemory, duint* newMemory, bool* isValid) const;

        FILETIME lastAccessed; //system user time

    private:
        TraceFileReader* mParent;
        std::vector<REGDUMP> mRegisters;
        QByteArray opcodes;
        std::vector<size_t> opcodeOffset;
        std::vector<unsigned char> opcodeSize;
        std::vector<size_t> memoryOperandOffset;
        std::vector<char> memoryFlags;
        std::vector<duint> memoryAddress;
        std::vector<duint> oldMemory;
        std::vector<duint> newMemory;
        std::vector<DWORD> threadId;
        unsigned long long length;
    };


    TraceFileParser* parser;
    std::map<Range, TraceFilePage, RangeCompare> pages;
    TraceFilePage* getPage(unsigned long long index);
};

#endif //TRACEFILEREADER_H
