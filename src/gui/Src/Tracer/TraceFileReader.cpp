#include "TraceFileReaderInternal.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>
#include "MiscUtil.h"
#include "StringUtil.h"

TraceFileReader::TraceFileReader(QObject* parent) : QObject(parent)
{
    length = 0;
    progress = 0;
    error = true;
    parser = nullptr;
    lastAccessedPage = nullptr;
    lastAccessedIndexOffset = 0;
    hashValue = 0;
    EXEPath.clear();
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
        traceFile.moveToThread(parser);
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
    hashValue = 0;
    EXEPath.clear();
    error = false;
}

bool TraceFileReader::Delete()
{
    if(parser != NULL)
    {
        parser->requestInterruption();
        parser->wait();
    }
    bool value = traceFile.remove();
    progress.store(0);
    length = 0;
    fileIndex.clear();
    hashValue = 0;
    EXEPath.clear();
    error = false;
    return value;
}

void TraceFileReader::parseFinishedSlot()
{
    if(!error)
        progress.store(100);
    else
        progress.store(0);
    delete parser;
    parser = nullptr;
    emit parseFinished();

    //for(auto i : fileIndex)
    //GuiAddLogMessage(QString("%1;%2;%3\r\n").arg(i.first).arg(i.second.first).arg(i.second.second).toUtf8().constData());
}

bool TraceFileReader::isError() const
{
    return error;
}

int TraceFileReader::Progress() const
{
    return progress.load();
}

unsigned long long TraceFileReader::Length() const
{
    return length;
}

duint TraceFileReader::HashValue() const
{
    return hashValue;
}

QString TraceFileReader::ExePath() const
{
    return EXEPath;
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
        if(cache->first.first >= index && cache->first.second <= index)
        {
            if(lastAccessedPage)
                GetSystemTimes(nullptr, nullptr, &lastAccessedPage->lastAccessed);
            lastAccessedPage = &cache->second;
            lastAccessedIndexOffset = cache->first.first;
            GetSystemTimes(nullptr, nullptr, &lastAccessedPage->lastAccessed);
            *base = lastAccessedIndexOffset;
            return lastAccessedPage;
        }
    }
    else if(index >= Length()) //Out of bound
        return nullptr;
    //page in
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
    size_t start = 0;
    size_t end = fileIndex.size() - 1;
    size_t middle = (start + end) / 2;
    std::pair<unsigned long long, Range>* fileOffset;
    while(true)
    {
        if(start == end || start == end - 1)
        {
            if(fileIndex[end].first <= index)
                fileOffset = &fileIndex[end];
            else
                fileOffset = &fileIndex[start];
            break;
        }
        if(fileIndex[middle].first > index)
            end = middle;
        else if(fileIndex[middle].first == index)
        {
            fileOffset = &fileIndex[middle];
            break;
        }
        else
            start = middle;
        middle = (start + end) / 2;
    }

    if(fileOffset->second.second + fileOffset->first >= index && fileOffset->first <= index)
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

//Parser

static bool checkKey(const QJsonObject & root, const QString & key, const QString & value)
{
    const auto obj = root.find(key);
    if(obj == root.constEnd())
        throw std::wstring(L"Unspecified");
    QJsonValue val = obj.value();
    if(val.isString())
        if(val.toString() == value)
            return true;
    return false;
}

void TraceFileParser::readFileHeader(TraceFileReader* that)
{
    LARGE_INTEGER header;
    if(that->traceFile.read((char*)&header, 8) != 8)
        throw std::wstring(L"Unspecified");
    if(header.LowPart != MAKEFOURCC('T', 'R', 'A', 'C'))
        throw std::wstring(L"File type mismatch");
    if(header.HighPart > 16384)
        throw std::wstring(L"Header info is too big");
    QByteArray jsonData = that->traceFile.read(header.HighPart);
    if(jsonData.size() != header.HighPart)
        throw std::wstring(L"Unspecified");
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
    if(jsonDoc.isNull())
        throw std::wstring(L"Unspecified");
    const QJsonObject jsonRoot = jsonDoc.object();

    const auto ver = jsonRoot.find("ver");
    if(ver == jsonRoot.constEnd())
        throw std::wstring(L"Unspecified");
    QJsonValue verVal = ver.value();
    if(verVal.toInt(0) != 1)
        throw std::wstring(L"Version not supported");
    checkKey(jsonRoot, "arch", ArchValue("x86", "x64"));
    checkKey(jsonRoot, "compression", "");
    const auto hashAlgorithmObj = jsonRoot.find("hashAlgorithm");
    if(hashAlgorithmObj != jsonRoot.constEnd())
    {
        QJsonValue hashAlgorithmVal = hashAlgorithmObj.value();
        if(hashAlgorithmVal.toString() == "murmurhash")
        {
            const auto hashObj = jsonRoot.find("hash");
            if(hashObj != jsonRoot.constEnd())
            {
                QJsonValue hashVal = hashObj.value();
                QString a = hashVal.toString();
                if(a.startsWith("0x"))
                {
                    a = a.mid(2);
#ifdef _WIN64
                    that->hashValue = a.toLongLong(nullptr, 16);
#else //x86
                    that->hashValue = a.toLong(nullptr, 16);
#endif //_WIN64
                }
            }
        }
    }
    const auto pathObj = jsonRoot.find("path");
    if(pathObj != jsonRoot.constEnd())
    {
        QJsonValue pathVal = pathObj.value();
        that->EXEPath = pathVal.toString();
    }
}

static bool readBlock(QFile & traceFile)
{
    if(!traceFile.isReadable())
        throw std::wstring(L"File is not readable");
    unsigned char blockType;
    unsigned char changedCountFlags[3]; //reg changed count, mem accessed count, flags
    if(traceFile.read((char*)&blockType, 1) != 1)
        throw std::wstring(L"Read block type failed");
    if(blockType == 0)
    {

        if(traceFile.read((char*)&changedCountFlags, 3) != 3)
            throw std::wstring(L"Read flags failed");
        //skipping: thread id, registers
        if(traceFile.seek(traceFile.pos() + ((changedCountFlags[2] & 0x80) ? 4 : 0) + (changedCountFlags[2] & 0x0F) + changedCountFlags[0] * (1 + sizeof(duint))) == false)
            throw std::wstring(L"Unspecified");
        QByteArray memflags;
        memflags = traceFile.read(changedCountFlags[1]);
        if(memflags.length() < changedCountFlags[1])
            throw std::wstring(L"Read memory flags failed");
        unsigned int skipOffset = 0;
        for(unsigned char i = 0; i < changedCountFlags[1]; i++)
            skipOffset += ((memflags[i] & 1) == 1) ? 2 : 3;
        if(traceFile.seek(traceFile.pos() + skipOffset * sizeof(duint)) == false)
            throw std::wstring(L"Unspecified");
        //Gathered information, build index
        if(changedCountFlags[0] == (FIELD_OFFSET(REGDUMP, lastError) + sizeof(DWORD)) / sizeof(duint))
            return true;
        else
            return false;
    }
    else
        throw std::wstring(L"Unsupported block type");
    return false;
}

void TraceFileParser::run()
{
    TraceFileReader* that = dynamic_cast<TraceFileReader*>(parent());
    unsigned long long index = 0;
    unsigned long long lastIndex = 0;
    if(that == NULL)
    {
        return; //Error
    }
    try
    {
        if(that->traceFile.size() == 0)
            throw std::wstring(L"File is empty");
        //Process file header
        readFileHeader(that);
        //Update progress
        that->progress.store(that->traceFile.pos() * 100 / that->traceFile.size());
        //Process file content
        while(!that->traceFile.atEnd())
        {
            quint64 blockStart = that->traceFile.pos();
            bool isPageBoundary = readBlock(that->traceFile);
            if(isPageBoundary)
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
        if(index > 0)
            that->fileIndex.back().second.second = index - (lastIndex - 1);
        that->error = false;
        that->length = index;
    }
    catch(const std::wstring & errReason)
    {
        //MessageBox(0, errReason.c_str(), L"debug", MB_ICONERROR);
        that->error = true;
    }
    catch(std::bad_alloc &)
    {
        that->error = true;
    }

    that->traceFile.moveToThread(that->thread());
}

void TraceFileReader::purgeLastPage()
{
    unsigned long long index = 0;
    unsigned long long lastIndex = 0;
    bool isBlockExist = false;
    if(length > 0)
    {
        index = fileIndex.back().first;
        const auto lastpage = pages.find(Range(index, index));
        if(lastpage != pages.cend())
        {
            //Purge last accessed page
            if(index == lastAccessedIndexOffset)
                lastAccessedPage = nullptr;
            //Remove last page from page cache
            pages.erase(lastpage);
        }
        //Seek start of last page
        traceFile.seek(fileIndex.back().second.first);
        //Remove last page from file index cache
        fileIndex.pop_back();
    }
    try
    {
        while(!traceFile.atEnd())
        {
            quint64 blockStart = traceFile.pos();
            bool isPageBoundary = readBlock(traceFile);
            if(isPageBoundary)
            {
                if(lastIndex != 0)
                    fileIndex.back().second.second = index - (lastIndex - 1);
                fileIndex.push_back(std::make_pair(index, TraceFileReader::Range(blockStart, 0)));
                lastIndex = index + 1;
                isBlockExist = true;
            }
            index++;
        }
        if(isBlockExist)
            fileIndex.back().second.second = index - (lastIndex - 1);
        error = false;
        length = index;
    }
    catch(std::wstring & errReason)
    {
        error = true;
    }
}

//TraceFilePage
TraceFilePage::TraceFilePage(TraceFileReader* parent, unsigned long long fileOffset, unsigned long long maxLength)
{
    DWORD lastThreadId = 0;
    union
    {
        REGDUMP registers;
        duint regwords[(FIELD_OFFSET(REGDUMP, lastError) + sizeof(DWORD)) / sizeof(duint)];
    };
    unsigned char changed[_countof(regwords)];
    duint regContent[_countof(regwords)];
    duint memAddress[MAX_MEMORY_OPERANDS];
    duint memOldContent[MAX_MEMORY_OPERANDS];
    duint memNewContent[MAX_MEMORY_OPERANDS];
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
                    if(changedCountFlags[0] > _countof(regwords)) //Bad count?
                        throw std::exception();
                    if(mParent->traceFile.read((char*)changed, changedCountFlags[0]) != changedCountFlags[0])
                        throw std::exception();
                    if(mParent->traceFile.read((char*)regContent, changedCountFlags[0] * sizeof(duint)) != changedCountFlags[0] * sizeof(duint))
                    {
                        throw std::exception();
                    }
                    for(int i = 0; i < changedCountFlags[0]; i++)
                    {
                        lastPosition = lastPosition + changed[i] + 1;
                        if(lastPosition < _countof(regwords) && lastPosition >= 0)
                            regwords[lastPosition] = regContent[i];
                        else //out of bounds?
                        {
                            throw std::exception();
                        }
                    }
                    mRegisters.push_back(registers);
                }
                if(changedCountFlags[1] > 0) //memory
                {
                    QByteArray memflags;
                    if(changedCountFlags[1] > _countof(memAddress)) //too many memory operands?
                        throw std::exception();
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
                    memoryOperandOffset.push_back(memOperandOffset);
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
