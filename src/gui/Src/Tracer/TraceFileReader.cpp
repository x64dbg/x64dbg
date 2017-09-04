#include "TraceFileReader.h"
#include <QThread>

TraceFileReader::TraceFileReader(QObject* parent) : QObject(parent)
{
    length = 0;
    progress = 0;
    error = true;
    parser = nullptr;
    lastAccessedPage = nullptr;
    lastAccessedIndexOffset = 0;
}

bool TraceFileReader::Open(const QString & fileName)
{
    if(parser != NULL && parser->isRunning()) //Trace file parser is busy
    {
        parser->requestInterruption();
        parser->wait();
    }
    error = true;
    traceFile.setFileName(fileName);
    traceFile.open(QFile::ReadOnly);
    if(traceFile.isReadable())
    {
        parser = new TraceFileParser(this);
        connect(parser, SIGNAL(finished()), this, SLOT(parseFinished()));
        progress.store(0);
        parser->start();
    }
    else
        return false;
}

void TraceFileReader::Close()
{
    if(parser != NULL)
    {
        parser->requestInterruption();
        parser->wait();
    }
    traceFile.close();
    progress.store(0);
    length = 0;
    fileIndex.clear();
}

void TraceFileReader::parseFinished()
{
    delete parser;
    parser = nullptr;
    progress.store(100);
}

bool TraceFileReader::isError()
{
    return error;
}

int TraceFileReader::Progress()
{
    return progress.load();
}

unsigned long long TraceFileReader::Length()
{
    return length;
}

TraceFilePage* TraceFileReader::getPage(unsigned long long index)
{
    if(index >= lastAccessedIndexOffset && index < lastAccessedIndexOffset + lastAccessedPage->Length())
    {
        return lastAccessedPage;
    }
    const auto cache = pages.find(Range(index, index));
    if(cache != pages.cend())
    {
        GetSystemTimes(nullptr, nullptr, &lastAccessedPage->lastAccessed);
        lastAccessedPage = &cache->second;
        GetSystemTimes(nullptr, nullptr, &lastAccessedPage->lastAccessed);
        return &lastAccessedPage;
    }
    else if(index >= Length())
        return nullptr;
    else //page in
    {
        if(pages.size() >= 2048) //TODO: trim resident pages based on system memory usage, instead of a hard limit.
        {
            FILETIME pageOutTime = pages.at(0).lastAccessed;
            int pageOutIndex = 0;
            for(int i = 1; i < pages.size(); i++)
            {
                if(pageOutTime.dwHighDateTime < pages.at(i).lastAccessed.dwHighDateTime || (pageOutTime.dwHighDateTime == pages.at(i).lastAccessed.dwHighDateTime && pageOutTime.dwLowDateTime < pages.at(i).lastAccessed.dwLowDateTime))
                {
                    pageOutTime = pages.at(i).lastAccessed;
                    pageOutIndex = i;
                }
            }
            pages.erase(pages.at(pageOutIndex));
        }
        //TODO: binary search fileIndex to get file offset, push a TraceFilePage into cache and return it.
    }
}

//Parser
void TraceFileReader::TraceFileParser::run()
{
    TraceFileReader* that = dynamic_cast<TraceFileReader*>(parent());
    unsigned long long index = 0;
    unsigned long long lastIndex = 0;
    if(that == NULL) SimpleErrorBox(this, "debug", "error");
    try
    {
        //Process file header
        //Update progress
        that->progress.store(that->traceFile.pos() * 100 / that->traceFile.size());
        //Process file content
        while(!that->traceFile.atEnd())
        {
            if(!that->traceFile.isReadable())
                throw std::exception();
            unsigned char blockType;
            unsigned char changedCountFlags[3]; //reg changed count, mem accessed count, flags
            that->traceFile.read((char*)&blockType, 1);
            if(blockType == 0)
            {
                quint64 blockStart = that->traceFile.pos();

                if(that->traceFile.read((char*)&blockType, 1) != 1)
                    throw std::exception();
                if(blockType != 0)
                    throw std::exception();
                if(that->traceFile.read((char*)&changedCountFlags, 3) != 3)
                    throw std::exception();
                //skipping: thread id, registers
                if(that->traceFile.seek(that->traceFile.pos() + ((changedCountFlags[2] & 0x80) ? 4 : 0) + (changedCountFlags[2] & 0x0F) + changedCountFlags[0] * (1 + sizeof(duint))) == false)
                    throw std::exception();
                QByteArray memflags;
                memflags = that->traceFile.read(changedCountFlags[1]);
                if(memflags.length() < changedCountFlags[1])
                    throw std::exception();
                unsigned int skipOffset = 0;
                for(unsigned char i = 0; i < changedCountFlags[1]; i++)
                    skipOffset += ((memflags[i] & 1) == 1) ? 2 : 3;
                if(that->traceFile.seek(that->traceFile.pos() + skipOffset * sizeof(duint)) == false)
                    throw std::exception();
                //Gathered information, build index
                if(changedCountFlags[0] == (sizeof(REGDUMP) - 128) / sizeof(duint))
                {
                    if(lastIndex != 0)
                        that->fileIndex.at(lastIndex - 1).second.second = index - (lastIndex - 1);
                    that->fileIndex.push_back(std::make_pair(index, Range(blockStart, 0)));
                    lastIndex = index + 1;
                    //Update progress
                    that->progress.store(that->traceFile.pos() * 100 / that->traceFile.size());
                    if(this->isInterruptionRequested() && !that->traceFile.atEnd()) //Cancel loading
                        throw std::exception();
                }
                index++;
            }
            else
                throw std::exception();
        }
    }
    catch(const std::exception &)
    {
        error = true;
    }
}

//TraceFilePage
TraceFileReader::TraceFilePage::TraceFilePage(TraceFileReader* parent, unsigned long long fileOffset, unsigned long long maxLength)
{
    DWORD lastThreadId = 0;
    union
    {
        REGDUMP registers;
        duint regwords[(sizeof(REGDUMP) - 128) / sizeof(duint)];
    };
    duint memAddress[32];
    duint memOldContent[32];
    duint memNewContent[32];
    size_t memOperandOffset = 0;
    mParent = parent;
    length = 0;
    GetSystemTimes(nullptr, nullptr, &lastAccessed); //system user time, no GetTickCount64() for XP compatibility.
    memset(registers, 0, sizeof(registers));
    try
    {
        if(mParent->traceFile.seek(fileOffset) == false)
            throw std::exception();
        //Process file content
        while(!that->traceFile.atEnd() && length < maxLength)
        {
            if(!that->traceFile.isReadable())
                throw std::exception();
            unsigned char blockType;
            unsigned char changedCountFlags[3]; //reg changed count, mem accessed count, flags
            mParent->traceFile.read((char*)&blockType, 1);
            if(blockType == 0)
            {
                if(mParent->traceFile.read((char*)&blockType, 1) != 1)
                    throw std::exception();
                if(blockType != 0)
                    throw std::exception();
                if(mParent->traceFile.read((char*)&changedCountFlags, 3) != 3)
                    throw std::exception();
                if(changedCountFlags[2] & 0x80) //Thread Id
                    mParent->traceFile.read((char*)&lastThreadId, 4);
                threadId.push_back(lastThreadId);
                if((changedCountFlags[2] & 0x0F) > 0) //Opcode
                {
                    QByteArray opcode = mParent->traceFile.read(changedCountFlags[2] & 0x0F);
                    if(opcode.isEmpty())
                        throw std::exception();
                    opcodeOffset.push_back(opcodes.size());
                    opcodeSize.push_back(opcode.size());
                    opcodes.append(opcode);
                }
                else
                    throw std::exception();
                if(changedCountFlags[0] > 0) //registers
                {
                    unsigned char lastPosition = 0;
                    QByteArray changed;
                    changed = mParent->traceFile.read(changedCountFlags[0]);
                    if(changed.size() != changedCountFlags[0])
                        throw std::exception();
                    duint* regContent = new duint[changedCountFlags[0]];
                    if(mParent->traceFile.read((char*)regContent, changedCountFlags[0] * sizeof(duint)) != changedCountFlags[0] * sizeof(duint))
                    {
                        delete[] regContent;
                        throw std::exception();
                    }
                    for(int i = 0; i < changedCountFlags[0]; i++)
                    {
                        lastPosition += changed[i];
                        regwords[lastPosition] = regContent[i];
                    }
                    delete[] regContent;
                    mRegisters.push_back(registers);
                }
                if(changedCountFlags[1] > 0) //memory
                {
                    QByteArray memflags;
                    memflags = mParent->traceFile.read(changedCountFlags[1]);
                    if(memflags.length() < changedCountFlags[1])
                        throw std::exception();
                    memoryOperandOffset.push_back(memOperandOffset);
                    memOperandOffset += changedCountFlags[1];
                    if(mParent->traceFile.read((char*)memAddress, sizeof(duint) * changedCountFlags[1]) != sizeof(duint) * changedCountFlags[1])
                        throw std::exception();
                    if(mParent->traceFile.read((char*)memOldContent, sizeof(duint) * changedCountFlags[1]) != sizeof(duint) * changedCountFlags[1])
                        throw std::exception();
                    for(unsigned char i = 0; i < changedCountFlags[1]; i++)
                    {
                        if((memflags[i] & 1) == 0)
                        {
                            if(mParent->traceFile.read((char*)&memNewContent[i], sizeof(duint)) != sizeof(duint))
                                throw std::exception();
                        }
                        else
                            memNewContent[i] = memOldContent[i];
                    }
                    for(unsigned char i = 0; i < changedCountFlags[1]; i++)
                    {
                        memoryFlags.push_back(memflags[i]);
                        memoryAddress.push_back(memAddress[i]);
                        oldMemory.push_back(memOldContent[i]);
                        newMemory.push_back(memNewContent[i]);
                    }
                }
                else
                    memoryOperandOffset.push_back(-1);
                length++;
            }
            else
                throw std::exception();
        }

    }
    catch(const std::exception &)
    {
        mParent->error = true;
    }
}
