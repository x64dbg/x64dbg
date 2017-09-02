#include "TraceRecord.h"
#include "capstone_wrapper.h"
#include "module.h"
#include "memory.h"
#include "threading.h"
#include "thread.h"
#include "disasm_helper.h"
#include "disasm_fast.h"
#include "plugin_loader.h"

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

void TraceRecordManager::TraceExecuteRecord(DISASM_INSTR & newInstruction)
{
    if(!isRunTraceEnabled())
        return;
    unsigned char WriteBuffer[2048];
    unsigned char* WriteBufferPtr = WriteBuffer;
    //Get current data
    REGDUMPDWORD newContext;
    //DISASM_INSTR newInstruction;
    DWORD newThreadId;
    duint newMemory[32];
    duint newMemoryAddress[32];
    duint oldMemory[32];
    unsigned char newMemoryArrayCount = 0;
    DbgGetRegDump(&newContext.registers);
    disasmget(newContext.registers.regcontext.cip, &newInstruction, true);
    newThreadId = ThreadGetId(hActiveThread);
    // Don't try to resolve memory values for lea and nop instructions
    if(!(memcmp(newInstruction.instruction, "lea ", 4) == 0 || memcmp(newInstruction.instruction, "nop ", 4) == 0 || memcmp(newInstruction.instruction, "prefetch", 8) == 0))
    {
        for(int i = 0; i < newInstruction.argcount; i++)
        {
            const DISASM_ARG & arg = newInstruction.arg[i];
            // TODO: Support SSE and AVX wide memory operands. They can be recorded in memory access log as multiple memory accesses of pointer size.
            // TODO: Support memory value of ??? for invalid memory access
            if(arg.type == arg_memory)
            {
                newMemory[newMemoryArrayCount] = arg.memvalue;
                newMemoryAddress[newMemoryArrayCount] = arg.value;
                newMemoryArrayCount++;
            }
        }
        assert(newMemoryArrayCount < 32);
    }
    if(rtPrevInstAvailable)
    {
        for(unsigned char i = 0; i < rtOldMemoryArrayCount; i++)
        {
            MemRead(rtOldMemoryAddress[i], oldMemory + i, sizeof(duint));
        }
        //Delta compress registers
        //Data layout is Structure of Arrays to gather the same type of data in continuous memory to improve RLE compression performance.
        //1byte:block type,1byte:reg changed count,1byte:memory accessed count,1byte:flags,4byte/none:threadid,string:opcode,1byte[]:position,4byte[]:regvalue,ptrbyte[]:address,1byte[]:flags,ptrbyte[]:oldmem,ptrbyte[]:newmem
        unsigned char changed = 0;
        for(unsigned char i = 0; i < _countof(rtOldContext.regdword); i++)
        {
            if(rtOldContext.regdword[i] != newContext.regdword[i])
                changed++;
        }
        unsigned char blockFlags = 0;
        if(newThreadId != rtOldThreadId)
            blockFlags = 0x80;
        blockFlags |= rtOldOpcodeSize;

        WriteBufferPtr[0] = 0; //1byte: block type
        WriteBufferPtr[1] = changed; //1byte: registers changed
        WriteBufferPtr[2] = rtOldMemoryArrayCount; //1byte: memory accesses count
        WriteBufferPtr[3] = blockFlags; //1byte: flags and opcode size
        WriteBufferPtr += 4;
        memcpy(WriteBufferPtr, rtOldOpcode, rtOldOpcodeSize);
        WriteBufferPtr += rtOldOpcodeSize;
        if(newThreadId != rtOldThreadId)
        {
            memcpy(WriteBufferPtr, &newThreadId, sizeof(newThreadId));
            WriteBufferPtr += sizeof(newThreadId);
        }
        for(unsigned char i = 0; i < _countof(rtOldContext.regdword); i++) //1byte: position
        {
            if(rtOldContext.regdword[i] != newContext.regdword[i])
            {
                WriteBufferPtr[0] = i;
                WriteBufferPtr++;
            }
        }
        for(unsigned char i = 0; i < _countof(rtOldContext.regdword); i++) //4byte: newvalue
        {
            if(rtOldContext.regdword[i] != newContext.regdword[i])
            {
                memcpy(WriteBufferPtr, &newContext.regdword[i], 4);
                WriteBufferPtr += 4;
            }
        }
        for(unsigned char i = 0; i < rtOldMemoryArrayCount; i++) //ptrbyte: address
        {
            memcpy(WriteBufferPtr, &rtOldMemoryAddress[i], sizeof(duint));
            WriteBufferPtr += sizeof(duint);
        }
        for(unsigned char i = 0; i < rtOldMemoryArrayCount; i++) //1byte: flags(reserved for invalid memory accesses or other uses)
        {
            WriteBufferPtr[0] = 0;
            WriteBufferPtr += 1;
        }
        for(unsigned char i = 0; i < rtOldMemoryArrayCount; i++) //ptrbyte: old content
        {
            memcpy(WriteBufferPtr, &rtOldMemory[i], sizeof(duint));
            WriteBufferPtr += sizeof(duint);
        }
        for(unsigned char i = 0; i < rtOldMemoryArrayCount; i++) //ptrbyte: new content
        {
            memcpy(WriteBufferPtr, &oldMemory[i], sizeof(duint));
            WriteBufferPtr += sizeof(duint);
        }
    }
    //Switch context buffers
    rtOldThreadId = newThreadId;
    rtOldContext = newContext;
    rtOldInstr = newInstruction;
    rtOldMemoryArrayCount = newMemoryArrayCount;
    memcpy(rtOldMemory, newMemory, sizeof(newMemory));
    memcpy(rtOldMemoryAddress, newMemoryAddress, sizeof(newMemoryAddress));
    memset(rtOldOpcode, 0, 16);
    rtOldOpcodeSize = newInstruction.instr_size & 0x0F;
    MemRead(newContext.registers.regcontext.cip, rtOldOpcode, rtOldOpcodeSize);
    //Write to file
    if(rtPrevInstAvailable)
    {
        if(WriteBufferPtr - WriteBuffer <= sizeof(WriteBuffer))
        {
            DWORD written;
            WriteFile(rtFile, WriteBuffer, WriteBufferPtr - WriteBuffer, &written, NULL);
            if(written < WriteBufferPtr - WriteBuffer) //Disk full?
            {
                CloseHandle(rtFile);
                dprintf(QT_TRANSLATE_NOOP("DBG", "Run trace has stopped unexpectedly because WriteFile() failed. GetLastError()= %X .\r\n"), GetLastError());
                rtEnabled = false;
            }
        }
        else
            __debugbreak(); // Buffer overrun?
    }
    rtPrevInstAvailable = true;
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
    InterlockedIncrement((volatile long*)&instructionCounter);
}

void TraceRecordManager::enableRunTrace(bool enabled, const char* fileName)
{
    if(enabled)
    {
        if(rtEnabled)
            enableRunTrace(false, NULL); //re-enable run trace
        rtFile = CreateFileW(StringUtils::Utf8ToUtf16(fileName).c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_DELETE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if(rtFile != INVALID_HANDLE_VALUE)
        {
            SetFilePointer(rtFile, 0, 0, FILE_END);
            rtPrevInstAvailable = false;
            rtEnabled = true;
        }
        else
            dprintf(QT_TRANSLATE_NOOP("DBG", "Cannot create run trace file. GetLastError()= %X .\r\n"), GetLastError());
    }
    else
    {
        if(rtEnabled)
        {
            CloseHandle(rtFile);
            rtPrevInstAvailable = false;
            rtEnabled = false;
        }
    }
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

bool TraceRecordManager::isRunTraceEnabled()
{
    return rtEnabled;
}

void _dbg_dbgtraceexecute(duint CIP)
{
    if(TraceRecord.getTraceRecordType(CIP) != TraceRecordManager::TraceRecordType::TraceRecordNone)
    {
        TraceRecord.increaseInstructionCounter();
        Capstone instruction;
        if(TraceRecord.isRunTraceEnabled())
        {
            DISASM_INSTR instr;
            disasmget(instruction, CIP, &instr);
            TraceRecord.TraceExecute(CIP, instruction.Size());
            TraceRecord.TraceExecuteRecord(instr);
        }
        else
        {
            BASIC_INSTRUCTION_INFO info;
            if(disasmfast(CIP, &info))
                TraceRecord.TraceExecute(CIP, info.size);
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

// When disabled, file name is not relevant and can be NULL
void _dbg_dbgenableRunTrace(bool enabled, const char* fileName)
{
    TraceRecord.enableRunTrace(enabled, fileName);
}