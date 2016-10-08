#include "TraceRecord.h"
#include "capstone_wrapper.h"
#include "module.h"
#include "memory.h"
#include "threading.h"

TraceRecordManager TraceRecord;

TraceRecordManager::TraceRecordManager() : instructionCounter(0)
{
    ModuleNames.emplace_back("");
}

TraceRecordManager::~TraceRecordManager()
{
    clear();
}

void TraceRecordManager::clear()
{
    EXCLUSIVE_ACQUIRE(LockTraceRecord);
    for(auto i = TraceRecord.begin(); i != TraceRecord.end(); ++i)
        efree(i->second.rawPtr, "TraceRecordManager");
    TraceRecord.clear();
    ModuleNames.clear();
    ModuleNames.emplace_back("");
}

bool TraceRecordManager::setTraceRecordType(duint pageAddress, TraceRecordType type)
{
    EXCLUSIVE_ACQUIRE(LockTraceRecord);
    pageAddress &= ~((duint)4096 - 1);
    auto pageInfo = TraceRecord.find(ModHashFromAddr(pageAddress));
    if(pageInfo == TraceRecord.end())
    {
        if(type != TraceRecordType::TraceRecordNone)
        {
            TraceRecordPage newPage;
            char modName[MAX_MODULE_SIZE];
            switch(type)
            {
            case TraceRecordBitExec:
                newPage.rawPtr = emalloc(4096 / 8, "TraceRecordManager");
                memset(newPage.rawPtr, 0, 4096 / 8);
                break;
            case TraceRecordByteWithExecTypeAndCounter:
                newPage.rawPtr = emalloc(4096, "TraceRecordManager");
                memset(newPage.rawPtr, 0, 4096);
                break;
            case TraceRecordWordWithExecTypeAndCounter:
                newPage.rawPtr = emalloc(4096 * 2, "TraceRecordManager");
                memset(newPage.rawPtr, 0, 4096 * 2);
                break;
            default:
                return false;
            }
            newPage.dataType = type;
            if(ModNameFromAddr(pageAddress, modName, true))
            {
                newPage.rva = pageAddress - ModBaseFromAddr(pageAddress);
                newPage.moduleIndex = getModuleIndex(std::string(modName));
            }
            else
                newPage.moduleIndex = ~0;

            auto inserted = TraceRecord.insert(std::make_pair(ModHashFromAddr(pageAddress), newPage));
            if(inserted.second == false) // we failed to insert new page into the map
            {
                efree(newPage.rawPtr);
                return false;
            }
            return true;
        }
        else
            return true;
    }
    else
    {
        if(type == TraceRecordType::TraceRecordNone)
        {
            if(pageInfo != TraceRecord.end())
            {
                efree(pageInfo->second.rawPtr, "TraceRecordManager");
                TraceRecord.erase(pageInfo);
            }
            return true;
        }
        else
            return pageInfo->second.dataType == type; //Can't covert between data types
    }
}

TraceRecordManager::TraceRecordType TraceRecordManager::getTraceRecordType(duint pageAddress)
{
    SHARED_ACQUIRE(LockTraceRecord);
    pageAddress &= ~((duint)4096 - 1);
    auto pageInfo = TraceRecord.find(ModHashFromAddr(pageAddress));
    if(pageInfo == TraceRecord.end())
        return TraceRecordNone;
    else
        return pageInfo->second.dataType;
}

void TraceRecordManager::TraceExecute(duint address, duint size)
{
    SHARED_ACQUIRE(LockTraceRecord);
    if(size == 0)
        return;
    duint base = address & ~((duint)4096 - 1);
    auto pageInfoIterator = TraceRecord.find(ModHashFromAddr(base));
    if(pageInfoIterator == TraceRecord.end())
        return;
    TraceRecordPage pageInfo;
    pageInfo = pageInfoIterator->second;
    duint offset = address - base;
    bool isMixed;
    if((offset + size) > 4096) // execution crossed page boundary, splitting into 2 sub calls. Noting that byte type may be mislabelled.
    {
        SHARED_RELEASE();
        TraceExecute(address, 4096 - offset);
        TraceExecute(base + 4096, size + offset - 4096);
        return;
    }
    isMixed = false;
    switch(pageInfo.dataType)
    {
    case TraceRecordType::TraceRecordBitExec:
        for(unsigned char i = 0; i < size; i++)
            *((char*)pageInfo.rawPtr + (i + offset) / 8) |= 1 << ((i + offset) % 8);
        break;

    case TraceRecordType::TraceRecordByteWithExecTypeAndCounter:
        for(unsigned char i = 0; i < size; i++)
        {
            TraceRecordByteType_2bit currentByteType;
            if(isMixed)
                currentByteType = TraceRecordByteType_2bit::_InstructionOverlapped;
            else if(i == 0)
                currentByteType = TraceRecordByteType_2bit::_InstructionHeading;
            else if(i == size - 1)
                currentByteType = TraceRecordByteType_2bit::_InstructionTailing;
            else
                currentByteType = TraceRecordByteType_2bit::_InstructionBody;

            char* data = (char*)pageInfo.rawPtr + offset + i;
            if(*data == 0)
            {
                *data = (char)currentByteType << 6 | 1;
            }
            else
            {
                isMixed |= (*data & 0xC0) >> 6 == currentByteType;
                *data = ((char)currentByteType << 6) | ((*data & 0x3F) == 0x3F ? 0x3F : (*data & 0x3F) + 1);
            }
        }
        if(isMixed)
            for(unsigned char i = 0; i < size; i++)
                *((char*)pageInfo.rawPtr + i + offset) |= 0xC0;
        break;

    case TraceRecordType::TraceRecordWordWithExecTypeAndCounter:
        for(unsigned char i = 0; i < size; i++)
        {
            TraceRecordByteType_2bit currentByteType;
            if(isMixed)
                currentByteType = TraceRecordByteType_2bit::_InstructionOverlapped;
            else if(i == 0)
                currentByteType = TraceRecordByteType_2bit::_InstructionHeading;
            else if(i == size - 1)
                currentByteType = TraceRecordByteType_2bit::_InstructionTailing;
            else
                currentByteType = TraceRecordByteType_2bit::_InstructionBody;

            short* data = (short*)pageInfo.rawPtr + offset + i;
            if(*data == 0)
            {
                *data = (char)currentByteType << 14 | 1;
            }
            else
            {
                isMixed |= (*data & 0xC0) >> 6 == currentByteType;
                *data = ((char)currentByteType << 14) | ((*data & 0x3FFF) == 0x3FFF ? 0x3FFF : (*data & 0x3FFF) + 1);
            }
        }
        if(isMixed)
            for(unsigned char i = 0; i < size; i++)
                *((short*)pageInfo.rawPtr + i + offset) |= 0xC000;
        break;

    default:
        break;
    }
}

unsigned int TraceRecordManager::getHitCount(duint address)
{
    SHARED_ACQUIRE(LockTraceRecord);
    duint base = address & ~((duint)4096 - 1);
    auto pageInfoIterator = TraceRecord.find(ModHashFromAddr(base));
    if(pageInfoIterator == TraceRecord.end())
        return 0;
    else
    {
        TraceRecordPage pageInfo = pageInfoIterator->second;
        duint offset = address - base;
        switch(pageInfo.dataType)
        {
        case TraceRecordType::TraceRecordBitExec:
            return ((char*)pageInfo.rawPtr)[offset / 8] & (1 << (offset % 8)) ? 1 : 0;
        case TraceRecordType::TraceRecordByteWithExecTypeAndCounter:
            return ((char*)pageInfo.rawPtr)[offset] & 0x3F;
        case TraceRecordType::TraceRecordWordWithExecTypeAndCounter:
            return ((short*)pageInfo.rawPtr)[offset] & 0x3FFF;
        default:
            return 0;
        }
    }
}

TraceRecordManager::TraceRecordByteType TraceRecordManager::getByteType(duint address)
{
    SHARED_ACQUIRE(LockTraceRecord);
    duint base = address & ~((duint)4096 - 1);
    auto pageInfoIterator = TraceRecord.find(ModHashFromAddr(base));
    if(pageInfoIterator == TraceRecord.end())
        return TraceRecordByteType::InstructionHeading;
    else
    {
        TraceRecordPage pageInfo = pageInfoIterator->second;
        duint offset = address - base;
        switch(pageInfo.dataType)
        {
        case TraceRecordType::TraceRecordBitExec:
        default:
            return TraceRecordByteType::InstructionHeading;
        case TraceRecordType::TraceRecordByteWithExecTypeAndCounter:
            return (TraceRecordByteType)((((char*)pageInfo.rawPtr)[offset] & 0xC0) >> 6);
        case TraceRecordType::TraceRecordWordWithExecTypeAndCounter:
            return (TraceRecordByteType)((((short*)pageInfo.rawPtr)[offset] & 0xC000) >> 14);
        }
    }
}

void TraceRecordManager::increaseInstructionCounter()
{
    InterlockedIncrement(&instructionCounter);
}

void TraceRecordManager::saveToDb(JSON root)
{
    EXCLUSIVE_ACQUIRE(LockTraceRecord);
    const JSON jsonTraceRecords = json_array();
    const char* byteToHex = "0123456789ABCDEF";
    for(auto i : TraceRecord)
    {
        JSON jsonObj = json_object();
        if(i.second.moduleIndex != ~0)
        {
            json_object_set_new(jsonObj, "module", json_string(ModuleNames[i.second.moduleIndex].c_str()));
            json_object_set_new(jsonObj, "rva", json_hex(i.second.rva));
        }
        else
        {
            json_object_set_new(jsonObj, "module", json_string(""));
            json_object_set_new(jsonObj, "rva", json_hex(i.first));
        }
        json_object_set_new(jsonObj, "type", json_hex((duint)i.second.dataType));
        auto ptr = (unsigned char*)i.second.rawPtr;
        duint size = 0;
        switch(i.second.dataType)
        {
        case TraceRecordType::TraceRecordBitExec:
            size = 4096 / 8;
            break;
        case TraceRecordType::TraceRecordByteWithExecTypeAndCounter:
            size = 4096;
            break;
        case TraceRecordType::TraceRecordWordWithExecTypeAndCounter:
            size = 4096 * 2;
            break;
        default:
            __debugbreak(); // We have encountered an error condition.
        }
        auto hex = StringUtils::ToCompressedHex(ptr, size);
        json_object_set_new(jsonObj, "data", json_string(hex.c_str()));
        json_array_append_new(jsonTraceRecords, jsonObj);
    }
    if(json_array_size(jsonTraceRecords))
        json_object_set(root, "tracerecord", jsonTraceRecords);

    // Notify garbage collector
    json_decref(jsonTraceRecords);
}

void TraceRecordManager::loadFromDb(JSON root)
{
    EXCLUSIVE_ACQUIRE(LockTraceRecord);
    // get the root object
    const JSON tracerecord = json_object_get(root, "tracerecord");

    // return if nothing found
    if(!tracerecord)
        return;

    size_t i;
    JSON value;
    json_array_foreach(tracerecord, i, value)
    {
        TraceRecordPage currentPage;
        size_t size;
        currentPage.dataType = (TraceRecordType)json_hex_value(json_object_get(value, "type"));
        currentPage.rva = (duint)json_hex_value(json_object_get(value, "rva"));
        switch(currentPage.dataType)
        {
        case TraceRecordType::TraceRecordBitExec:
            size = 4096 / 8;
            break;
        case TraceRecordType::TraceRecordByteWithExecTypeAndCounter:
            size = 4096;
            break;
        case TraceRecordType::TraceRecordWordWithExecTypeAndCounter:
            size = 4096 * 2;
            break;
        default:
            size = 0;
            break;
        }
        if(size != 0)
        {
            currentPage.rawPtr = emalloc(size, "TraceRecordManager");
            const char* p = json_string_value(json_object_get(value, "data"));
            std::vector<unsigned char> data;
            if(StringUtils::FromCompressedHex(p, data) && data.size() == size)
            {
                memcpy(currentPage.rawPtr, data.data(), size);
                const char* moduleName = json_string_value(json_object_get(value, "module"));
                duint key;
                if(*moduleName)
                {
                    currentPage.moduleIndex = getModuleIndex(std::string(moduleName));
                    key = currentPage.rva + ModHashFromName(moduleName);
                }
                else
                {
                    currentPage.moduleIndex = ~0;
                    key = currentPage.rva;
                }
                TraceRecord.insert(std::make_pair(key, currentPage));
            }
            else
                efree(currentPage.rawPtr, "TraceRecordManager");
        }
    }
}

unsigned int TraceRecordManager::getModuleIndex(const String & moduleName)
{
    auto iterator = std::find(ModuleNames.begin(), ModuleNames.end(), moduleName);
    if(iterator != ModuleNames.end())
        return (unsigned int)(iterator - ModuleNames.begin());
    else
    {
        ModuleNames.push_back(moduleName);
        return (unsigned int)(ModuleNames.size() - 1);
    }
}

void _dbg_dbgtraceexecute(duint CIP)
{
    if(TraceRecord.getTraceRecordType(CIP) != TraceRecordManager::TraceRecordType::TraceRecordNone)
    {
        unsigned char buffer[MAX_DISASM_BUFFER];
        if(MemRead(CIP, buffer, MAX_DISASM_BUFFER))
        {
            TraceRecord.increaseInstructionCounter();
            Capstone instruction;
            instruction.Disassemble(CIP, buffer, MAX_DISASM_BUFFER);
            TraceRecord.TraceExecute(CIP, instruction.Size());
        }
        else
        {
            // if we reaches here, then the executable had executed an invalid address. Don't trace it.
        }
    }
    else
        TraceRecord.increaseInstructionCounter();
}

unsigned int _dbg_dbggetTraceRecordHitCount(duint address)
{
    return TraceRecord.getHitCount(address);
}

TRACERECORDBYTETYPE _dbg_dbggetTraceRecordByteType(duint address)
{
    return (TRACERECORDBYTETYPE)TraceRecord.getByteType(address);
}

bool _dbg_dbgsetTraceRecordType(duint pageAddress, TRACERECORDTYPE type)
{
    return TraceRecord.setTraceRecordType(pageAddress, (TraceRecordManager::TraceRecordType)type);
}

TRACERECORDTYPE _dbg_dbggetTraceRecordType(duint pageAddress)
{
    return (TRACERECORDTYPE)TraceRecord.getTraceRecordType(pageAddress);
}
