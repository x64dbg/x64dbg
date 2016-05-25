#include "TraceRecord.h"
#include "capstone_wrapper.h"
#include "module.h"
#include "memory.h"
#include "threading.h"
#include "console.h"
#include <algorithm>

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
    for(auto i = TraceRecord.begin(); i != TraceRecord.end(); i++)
        efree(i->second.rawPtr, "TraceRecordManager");
    TraceRecord.clear();
    ModuleNames.clear();
    ModuleNames.emplace_back("");
}

bool TraceRecordManager::setTraceRecordType(duint pageAddress, TraceRecordType type)
{
    EXCLUSIVE_ACQUIRE(LockTraceRecord);
    pageAddress &= ~((duint)4096-1);
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
                free(newPage.rawPtr);
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
    const char *byteToHex = "0123456789ABCDEF";
    for (auto i : TraceRecord)
    {
        JSON jsonObj = json_object();
        duint j;
        json_object_set_new(jsonObj, "idx", json_hex(i.first));
        if (i.second.moduleIndex != ~0)
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
        char *ptr = (char*)i.second.rawPtr;
        duint size = 0;
        char *temp;
        switch (i.second.dataType)
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
        temp = (char*)emalloc(size * 2 + 1, "TraceRecordManager::saveToDb");
        for (j = 0; j < size; j++)
        {
            temp[j * 2 + 1] = byteToHex[ptr[j] & 0xF];
            temp[j * 2] = byteToHex[(ptr[j] & 0xF0) >> 4];
        }
        temp[size * 2] = 0;
        json_object_set_new(jsonObj, "data", json_string(temp));
        efree(temp, "TraceRecordManager::saveToDb");
        json_array_append_new(jsonTraceRecords, jsonObj);
    }
    json_object_set_new(root, "tracerecord", jsonTraceRecords);
}

void TraceRecordManager::loadFromDb(JSON root)
{
    clear();
    EXCLUSIVE_ACQUIRE(LockTraceRecord);
    // get the root object
    const JSON tracerecord = json_object_get(root, "tracerecord");

    // return if nothing found
    if (!tracerecord)
        return;

    size_t i;
    JSON value;
    json_array_foreach(tracerecord, i, value)
    {
        TraceRecordPage currentPage;
        size_t size;
        currentPage.dataType = (TraceRecordType)json_hex_value(json_object_get(value, "type"));
        currentPage.rva = json_hex_value(json_object_get(value, "rva"));
        switch (currentPage.dataType)
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
        if (size != 0)
        {
            currentPage.rawPtr = emalloc(size, "TraceRecordManager");
            const char *p = json_string_value(json_object_get(value, "data"));
            char *read_ptr = (char*)currentPage.rawPtr;
            const char *c;
            for (c = p; c < p + size * 2 && *c != '\0'; c += 2, read_ptr++)
            {
                if (c[1] >= '0' && c[1] <= '9')
                    *read_ptr = c[1] - '0';
                else if (c[1] >= 'A' && c[0] <= 'F')
                    *read_ptr = c[1] - 'A' + 10;
                else
                    break;
                if (c[0] >= '0' && c[0] <= '9')
                    *read_ptr |= (c[0] - '0') << 4;
                else if (c[0] >= 'A' && c[0] <= 'F')
                    *read_ptr |= (c[0] - 'A' + 10) << 4;
                else
                    break;
            }
            if (c != p + size * 2)
            {
                efree(currentPage.rawPtr, "TraceRecordManager");
            }
            else
            {
                const char* moduleName = json_string_value(json_object_get(value, "module"));
                duint key;
                dprintf("%s\n", moduleName);
                if (*moduleName)
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
        }
    }
}

unsigned int TraceRecordManager::getModuleIndex(std::string moduleName)
{
    auto iterator = std::find(ModuleNames.begin(), ModuleNames.end(), moduleName);
    if (iterator != ModuleNames.end())
        return iterator - ModuleNames.begin();
    else
    {
        ModuleNames.push_back(moduleName);
        return ModuleNames.size() - 1;
    }
}

void _dbg_dbgtraceexecute(duint CIP)
{
    if(TraceRecord.getTraceRecordType(CIP) != TraceRecordManager::TraceRecordType::TraceRecordNone)
    {
        unsigned char buffer[16];
        duint size;
        if(MemRead(CIP, buffer, 16))
        {
            BASIC_INSTRUCTION_INFO basicInfo;
            TraceRecord.increaseInstructionCounter();
            DbgDisasmFastAt(CIP, &basicInfo);
            TraceRecord.TraceExecute(CIP, basicInfo.size);
        }
        else
        {
            duint base = MemFindBaseAddr(CIP, &size);
            if(CIP - base + 16 > size) // Corner case where CIP is near the end of the page
            {
                size = base + size - CIP;
                if(MemRead(CIP, buffer, size))
                {
                    BASIC_INSTRUCTION_INFO basicInfo;
                    TraceRecord.increaseInstructionCounter();
                    DbgDisasmFastAt(CIP, &basicInfo);
                    TraceRecord.TraceExecute(CIP, basicInfo.size);
                    return;
                }
            }
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
