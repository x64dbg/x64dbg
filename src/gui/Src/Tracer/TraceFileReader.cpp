#include "TraceFileReaderInternal.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>
#include "MiscUtil.h"
#include "StringUtil.h"
#include <sysinfoapi.h>

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

    int maxModuleSize = (int)ConfigUint("Disassembler", "MaxModuleSize");
    mDisasm = new QZydis(maxModuleSize, Bridge::getArch());
    connect(Config(), SIGNAL(tokenizerConfigUpdated()), this, SLOT(tokenizerUpdatedSlot()));
    connect(Config(), SIGNAL(colorsUpdated()), this, SLOT(tokenizerUpdatedSlot()));
}

TraceFileReader::~TraceFileReader()
{
    delete mDisasm;
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

// Return if the file read was error
bool TraceFileReader::isError() const
{
    return error;
}

// Return 100 when loading is completed
int TraceFileReader::Progress() const
{
    return progress.load();
}

// Return the count of instructions
unsigned long long TraceFileReader::Length() const
{
    return length;
}

QString TraceFileReader::getIndexText(unsigned long long index) const
{
    QString indexString;
    indexString = QString::number(index, 16).toUpper();
    if(length < 16)
        return indexString;
    int digits;
    digits = floor(log2(length - 1) / 4) + 1;
    digits -= indexString.size();
    while(digits > 0)
    {
        indexString = '0' + indexString;
        digits = digits - 1;
    }
    return indexString;
}

// Return the hash value of executable to be matched against current executable
duint TraceFileReader::HashValue() const
{
    return hashValue;
}

// Return the executable name of executable
const QString & TraceFileReader::ExePath() const
{
    return EXEPath;
}

// Return the registers context at a given index
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

// Return the opcode at a given index. buffer must be 16 bytes long.
void TraceFileReader::OpCode(unsigned long long index, unsigned char* buffer, int* opcodeSize)
{
    unsigned long long base;
    TraceFilePage* page = getPage(index, &base);
    if(page == nullptr)
    {
        memset(buffer, 0, 16);
        *opcodeSize = 0;
        return;
    }
    else
        page->OpCode(index - base, buffer, opcodeSize);
}

// Return the disassembled instruction at a given index.
const Instruction_t & TraceFileReader::Instruction(unsigned long long index)
{
    unsigned long long base;
    TraceFilePage* page = getPage(index, &base);
    // The caller must guarantee page is not null, most likely they have already called some other getters.
    return page->Instruction(index - base, *mDisasm);
}

// Return the thread id at a given index
DWORD TraceFileReader::ThreadId(unsigned long long index)
{
    unsigned long long base;
    TraceFilePage* page = getPage(index, &base);
    if(page == nullptr)
        return 0;
    else
        return page->ThreadId(index - base);
}

// Return the number of recorded memory accesses at a given index
int TraceFileReader::MemoryAccessCount(unsigned long long index)
{
    unsigned long long base;
    TraceFilePage* page = getPage(index, &base);
    if(page == nullptr)
        return 0;
    else
        return page->MemoryAccessCount(index - base);
}

// Return the memory access info at a given index
void TraceFileReader::MemoryAccessInfo(unsigned long long index, duint* address, duint* oldMemory, duint* newMemory, bool* isValid)
{
    unsigned long long base;
    TraceFilePage* page = getPage(index, &base);
    if(page == nullptr)
        return;
    else
        return page->MemoryAccessInfo(index - base, address, oldMemory, newMemory, isValid);
}

static size_t getMaxCachedPages()
{
#ifdef _WIN64
    MEMORYSTATUSEX meminfo;
    memset(&meminfo, 0, sizeof(meminfo));
    meminfo.dwLength = sizeof(meminfo);
    if(GlobalMemoryStatusEx(&meminfo))
    {
        meminfo.ullAvailPhys >>= 20;
        if(meminfo.ullAvailPhys >= 4096) // more than 4GB free memory!
            return 2048;
        else if(meminfo.ullAvailPhys <= 1024) // less than 1GB free memory!
            return 100;
        else
            return 512;
    } // GlobalMemoryStatusEx failed?
    else
        return 100;
#else //x86
    return 100;
#endif
}

// Used internally to get the page for the given index and read from disk if necessary
TraceFilePage* TraceFileReader::getPage(unsigned long long index, unsigned long long* base)
{
    // Try to access the most recently used page
    if(lastAccessedPage)
    {
        if(index >= lastAccessedIndexOffset && index < lastAccessedIndexOffset + lastAccessedPage->Length())
        {
            *base = lastAccessedIndexOffset;
            return lastAccessedPage;
        }
    }
    // Try to access pages in memory
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
    // Remove an oldest page from system memory to make room for a new one.
    size_t maxPages = getMaxCachedPages();
    while(pages.size() >= maxPages)
    {
        FILETIME pageOutTime = pages.begin()->second.lastAccessed;
        Range pageOutIndex = pages.begin()->first;
        for(auto & i : pages)
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
    // Read the requested page from disk and return
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


void TraceFileReader::tokenizerUpdatedSlot()
{
    mDisasm->UpdateConfig();
    for(auto & i : pages)
        i.second.updateInstructions();
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
    bool ok;
    if(that->traceFile.read((char*)&header, 8) != 8)
        throw std::wstring(L"Unspecified");
    if(header.LowPart != MAKEFOURCC('T', 'R', 'A', 'C'))
        throw std::wstring(L"File type mismatch");
    if(header.HighPart > 16384)
        throw std::wstring(L"Header info is too big");
    QByteArray jsonData = that->traceFile.read(header.HighPart);
    if(jsonData.size() != header.HighPart)
        throw std::wstring(L"JSON header is corrupted");
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
    if(jsonDoc.isNull())
        throw std::wstring(L"JSON header is corrupted");
    const QJsonObject jsonRoot = jsonDoc.object();

    const auto ver = jsonRoot.find("ver");
    if(ver == jsonRoot.constEnd())
        throw std::wstring(L"Version not supported");
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
                    that->hashValue = a.toLongLong(&ok, 16);
#else //x86
                    that->hashValue = a.toLong(&ok, 16);
#endif //_WIN64
                    if(!ok)
                        that->hashValue = 0;
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
        auto filesize = that->traceFile.size();
        if(filesize == 0)
            throw std::wstring(L"File is empty");
        //Process file header
        readFileHeader(that);
        //Update progress
        that->progress.store(that->traceFile.pos() * 100 / filesize);
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
                that->progress.store(that->traceFile.pos() * 100 / filesize);
                if(that->progress == 100)
                    that->progress = 99;
                if(this->isInterruptionRequested() && !that->traceFile.atEnd()) //Cancel loading
                    throw std::wstring(L"Canceled");
            }
            index++;
        }
        if(index > 0)
            that->fileIndex.back().second.second = index - (lastIndex - 1);
        that->error = false;
        that->length = index;
        that->progress = 100;
    }
    catch(const std::wstring & errReason)
    {
        Q_UNUSED(errReason);
        //MessageBox(0, errReason.c_str(), L"debug", MB_ICONERROR);
        that->error = true;
    }
    catch(std::bad_alloc &)
    {
        that->error = true;
    }

    that->traceFile.moveToThread(that->thread());
}

// Remove last page from memory and read from disk again to show updates
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
        Q_UNUSED(errReason);
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

const REGDUMP & TraceFilePage::Registers(unsigned long long index) const
{
    return mRegisters.at(index);
}

void TraceFilePage::OpCode(unsigned long long index, unsigned char* buffer, int* opcodeSize) const
{
    *opcodeSize = this->opcodeSize.at(index);
    memcpy(buffer, opcodes.constData() + opcodeOffset.at(index), *opcodeSize);
}

const Instruction_t & TraceFilePage::Instruction(unsigned long long index, QZydis & mDisasm)
{
    if(instructions.size() == 0)
    {
        instructions.reserve(length);
        for(unsigned long long i = 0; i < length; i++)
        {
            instructions.emplace_back(mDisasm.DisassembleAt((const byte_t*)opcodes.constData() + opcodeOffset.at(i), opcodeSize.at(i), 0, Registers(i).regcontext.cip, false));
        }
    }
    return instructions.at(index);
}

DWORD TraceFilePage::ThreadId(unsigned long long index) const
{
    return threadId.at(index);
}

int TraceFilePage::MemoryAccessCount(unsigned long long index) const
{
    size_t a = memoryOperandOffset.at(index);
    if(index == length - 1)
        return (int)(memoryAddress.size() - a);
    else
        return (int)(memoryOperandOffset.at(index + 1) - a);
}

void TraceFilePage::MemoryAccessInfo(unsigned long long index, duint* address, duint* oldMemory, duint* newMemory, bool* isValid) const
{
    auto count = MemoryAccessCount(index);
    auto base = memoryOperandOffset.at(index);
    for(int i = 0; i < count; i++)
    {
        address[i] = memoryAddress.at(base + i);
        oldMemory[i] = this->oldMemory.at(base + i);
        newMemory[i] = this->newMemory.at(base + i);
        isValid[i] = true; // proposed flag
    }
}

void TraceFilePage::updateInstructions()
{
    instructions.clear();
}
