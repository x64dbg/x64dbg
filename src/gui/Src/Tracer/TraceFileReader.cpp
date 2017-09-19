#include "TraceFileReaderInternal.h"
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
        connect(parser, SIGNAL(finished()), this, SLOT(parseFinishedSlot()));
        progress.store(0);
        parser->start();
        return true;
    }
    else
    {
        progress.store(0);
        emit parseFinished();
        return false;
    }
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

void TraceFileReader::parseFinishedSlot()
{
    delete parser;
    parser = nullptr;
    progress.store(100);
    emit parseFinished();

    //for(auto i : fileIndex)
    //GuiAddLogMessage(QString("%1;%2;%3\r\n").arg(i.first).arg(i.second.first).arg(i.second.second).toUtf8().constData());
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


REGDUMP TraceFileReader::Registers(unsigned long long index)
{
    unsigned long long base;
    TraceFilePage* page = getPage(index, &base);
    if(page == nullptr)
    {
        REGDUMP registers;
        memset(&registers, 0, sizeof(registers));
        return registers;
    }
    else
        return page->Registers(index - base);
}

void TraceFileReader::OpCode(unsigned long long index, unsigned char* buffer, int* opcodeSize)
{
    unsigned long long base;
    TraceFilePage* page = getPage(index, &base);
    if(page == nullptr)
    {
        memset(buffer, 0, 16);
        return;
    }
    else
        page->OpCode(index - base, buffer, opcodeSize);
}

DWORD TraceFileReader::ThreadId(unsigned long long index)
{
    unsigned long long base;
    TraceFilePage* page = getPage(index, &base);
    if(page == nullptr)
        return 0;
    else
        return page->ThreadId(index - base);
}

int TraceFileReader::MemoryAccessCount(unsigned long long index)
{
    unsigned long long base;
    TraceFilePage* page = getPage(index, &base);
    if(page == nullptr)
        return 0;
    else
        return page->MemoryAccessCount(index - base);
}

void TraceFileReader::MemoryAccessInfo(unsigned long long index, duint* address, duint* oldMemory, duint* newMemory, bool* isValid)
{
    unsigned long long base;
    TraceFilePage* page = getPage(index, &base);
    if(page == nullptr)
        return;
    else
        return page->MemoryAccessInfo(index - base, address, oldMemory, newMemory, isValid);
}

TraceFilePage* TraceFileReader::getPage(unsigned long long index, unsigned long long* base)
{
    if(lastAccessedPage)
    {
        if(index >= lastAccessedIndexOffset && index < lastAccessedIndexOffset + lastAccessedPage->Length())
        {
            *base = lastAccessedIndexOffset;
            return lastAccessedPage;
        }
    }
    const auto cache = pages.find(Range(index, index));
    if(cache != pages.cend())
    {
        if(lastAccessedPage)
            GetSystemTimes(nullptr, nullptr, &lastAccessedPage->lastAccessed);
        lastAccessedPage = &cache->second;
        lastAccessedIndexOffset = cache->first.first;
        GetSystemTimes(nullptr, nullptr, &lastAccessedPage->lastAccessed);
        *base = lastAccessedIndexOffset;
        return lastAccessedPage;
    }
    else if(index >= Length()) //Out of bound
        return nullptr;
    else //page in
    {
        if(pages.size() >= 2048) //TODO: trim resident pages based on system memory usage, instead of a hard limit.
        {
            FILETIME pageOutTime = pages.begin()->second.lastAccessed;
            Range pageOutIndex = pages.begin()->first;
            for(auto i : pages)
            {
                if(pageOutTime.dwHighDateTime < i.second.lastAccessed.dwHighDateTime || (pageOutTime.dwHighDateTime == i.second.lastAccessed.dwHighDateTime && pageOutTime.dwLowDateTime < i.second.lastAccessed.dwLowDateTime))
                {
                    pageOutTime = i.second.lastAccessed;
                    pageOutIndex = i.first;
                }
            }
            pages.erase(pageOutIndex);
        }
        //binary search fileIndex to get file offset, push a TraceFilePage into cache and return it.
        auto fileOffset = std::lower_bound(fileIndex.begin(), fileIndex.end(), index, [](std::pair<unsigned long long, Range> & b, unsigned long long a)
        {
            return b.first < a;
        });
        if(fileOffset->second.second + fileOffset->first > index)
        {
            pages.insert(std::make_pair(Range(fileOffset->first, fileOffset->first + fileOffset->second.second - 1), TraceFilePage(this, fileOffset->second.first, fileOffset->second.second)));
            const auto newPage = pages.find(Range(index, index));
            if(newPage != pages.cend())
            {
                if(lastAccessedPage)
                    GetSystemTimes(nullptr, nullptr, &lastAccessedPage->lastAccessed);
                lastAccessedPage = &newPage->second;
                lastAccessedIndexOffset = newPage->first.first;
                GetSystemTimes(nullptr, nullptr, &lastAccessedPage->lastAccessed);
                *base = lastAccessedIndexOffset;
                return lastAccessedPage;
            }
            else
            {
                GuiAddLogMessage("PAGEFAULT2\r\n"); //debug
                return nullptr; //???
            }
        }
        else
        {
            GuiAddLogMessage("PAGEFAULT1\r\n"); //debug
            return nullptr; //???
        }
    }
}

//Parser
void TraceFileParser::run()
{
    TraceFileReader* that = dynamic_cast<TraceFileReader*>(parent());
    unsigned long long index = 0;
    unsigned long long lastIndex = 0;
    if(that == NULL)
        return; //Error
    try
    {
        //Process file header
        //Update progress
        that->progress.store(that->traceFile.pos() * 100 / that->traceFile.size());
        //Process file content
        while(!that->traceFile.atEnd())
        {
            if(!that->traceFile.isReadable())
                throw std::wstring(L"File is not readable");
            unsigned char blockType;
            unsigned char changedCountFlags[3]; //reg changed count, mem accessed count, flags
            if(that->traceFile.read((char*)&blockType, 1) != 1)
                throw std::wstring(L"Read block type failed");
            if(blockType == 0)
            {
                quint64 blockStart = that->traceFile.pos() - 1;

                if(that->traceFile.read((char*)&changedCountFlags, 3) != 3)
                    throw std::wstring(L"Read flags failed");
                //skipping: thread id, registers
                if(that->traceFile.seek(that->traceFile.pos() + ((changedCountFlags[2] & 0x80) ? 4 : 0) + (changedCountFlags[2] & 0x0F) + changedCountFlags[0] * (1 + sizeof(duint))) == false)
                    throw std::wstring(L"Unspecified");
                QByteArray memflags;
                memflags = that->traceFile.read(changedCountFlags[1]);
                if(memflags.length() < changedCountFlags[1])
                    throw std::wstring(L"Read memory flags failed");
                unsigned int skipOffset = 0;
                for(unsigned char i = 0; i < changedCountFlags[1]; i++)
                    skipOffset += ((memflags[i] & 1) == 1) ? 2 : 3;
                if(that->traceFile.seek(that->traceFile.pos() + skipOffset * sizeof(duint)) == false)
                    throw std::wstring(L"Unspecified");
                //Gathered information, build index
                if(changedCountFlags[0] == (sizeof(REGDUMP) - 128) / sizeof(duint))
                {
                    if(lastIndex != 0)
                        that->fileIndex.back().second.second = index - (lastIndex - 1);
                    that->fileIndex.push_back(std::make_pair(index, TraceFileReader::Range(blockStart, 0)));
                    lastIndex = index + 1;
                    //Update progress
                    that->progress.store(that->traceFile.pos() * 100 / that->traceFile.size());
                    if(this->isInterruptionRequested() && !that->traceFile.atEnd()) //Cancel loading
                        throw std::wstring(L"Canceled");
                }
                index++;
            }
            else
                throw std::wstring(L"Unsupported block type");
        }
        that->fileIndex.back().second.second = index - (lastIndex - 1);
        that->error = false;
        that->length = index;
    }
    catch(const std::wstring & errReason)
    {
        //MessageBox(0, errReason.c_str(), L"debug", MB_ICONERROR);
        that->error = true;
    }
}

//TraceFilePage
TraceFilePage::TraceFilePage(TraceFileReader* parent, unsigned long long fileOffset, unsigned long long maxLength)
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
    memset(&registers, 0, sizeof(registers));
    try
    {
        if(mParent->traceFile.seek(fileOffset) == false)
            throw std::exception();
        //Process file content
        while(!mParent->traceFile.atEnd() && length < maxLength)
        {
            if(!mParent->traceFile.isReadable())
                throw std::exception();
            unsigned char blockType;
            unsigned char changedCountFlags[3]; //reg changed count, mem accessed count, flags
            mParent->traceFile.read((char*)&blockType, 1);
            if(blockType == 0)
            {
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
                    int lastPosition = -1;
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
                        lastPosition = lastPosition + changed[i] + 1;
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

unsigned long long TraceFilePage::Length() const
{
    return length;
}

REGDUMP TraceFilePage::Registers(unsigned long long index) const
{
    return mRegisters.at(index);
}

void TraceFilePage::OpCode(unsigned long long index, unsigned char* buffer, int* opcodeSize) const
{
    *opcodeSize = this->opcodeSize.at(index);
    memcpy(buffer, opcodes.constData() + opcodeOffset.at(index), *opcodeSize);
}

DWORD TraceFilePage::ThreadId(unsigned long long index) const
{
    return threadId.at(index);
}

int TraceFilePage::MemoryAccessCount(unsigned long long index) const
{
    size_t a = memoryOperandOffset.at(index);
    if(index == length - 1)
        return memoryAddress.size() - a;
    else
        return memoryOperandOffset.at(index + 1) - a;
}

void TraceFilePage::MemoryAccessInfo(unsigned long long index, duint* address, duint* oldMemory, duint* newMemory, bool* isValid) const
{
    auto count = MemoryAccessCount(index);
    auto base = memoryOperandOffset.at(index);
    for(size_t i = 0; i < count; i++)
    {
        address[i] = memoryAddress.at(base + i);
        oldMemory[i] = this->oldMemory.at(base + i);
        newMemory[i] = this->newMemory.at(base + i);
        isValid[i] = true; // proposed flag
    }
}
