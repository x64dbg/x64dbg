#include "TraceRecord.h"
#include "module.h"
#include "memory.h"
#include "threading.h"
#include "thread.h"
#include "disasm_helper.h"
#include "disasm_fast.h"
#include "plugin_loader.h"
#include "stringformat.h"
#include "value.h"

#define MAX_INSTRUCTIONS_TRACED_FULL_REG_DUMP 512

TraceRecordManager TraceRecord;

TraceRecordManager::TraceRecordManager()
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
            {
                newPage.rva = pageAddress;
                newPage.moduleIndex = ~0;
            }

            auto inserted = TraceRecord.emplace(ModHashFromAddr(pageAddress), newPage);
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

//See https://www.felixcloutier.com/x86/FXSAVE.html, max 512 bytes
#define memoryContentSize 512

static void HandleZydisOperand(const Zydis & zydis, int opindex, DISASM_ARGTYPE* argType, duint* value, unsigned char memoryContent[memoryContentSize], unsigned char* memorySize)
{
    *value = (duint)zydis.ResolveOpValue(opindex, [&zydis](ZydisRegister reg) -> uint64_t
    {
        auto regName = zydis.RegName(reg);
        return regName ? getregister(nullptr, regName) : 0; //TODO: temporary needs enums + caching
    });
    const auto & op = zydis[opindex];
    switch(op.type)
    {
    case ZYDIS_OPERAND_TYPE_REGISTER:
        *argType = arg_normal;
        break;

    case ZYDIS_OPERAND_TYPE_IMMEDIATE:
        *argType = arg_normal;
        break;

    case ZYDIS_OPERAND_TYPE_POINTER:
        *argType = arg_normal;
        break;

    case ZYDIS_OPERAND_TYPE_MEMORY:
    {
        *argType = arg_memory;
        const auto & mem = op.mem;
        if(mem.segment == ArchValue(ZYDIS_REGISTER_FS, ZYDIS_REGISTER_GS))
        {
            *value += ThreadGetLocalBase(ThreadGetId(hActiveThread));
        }
        *memorySize = op.size / 8;
        if(*memorySize <= memoryContentSize && DbgMemIsValidReadPtr(*value))
        {
            MemRead(*value, memoryContent, std::max(op.size / 8, (int)sizeof(duint)));
        }
    }
    break;

    default:
        __debugbreak();
    }
}

void TraceRecordManager::TraceExecuteRecord(const Zydis & newInstruction)
{
    if(!isTraceRecordingEnabled())
        return;
    unsigned char WriteBuffer[3072];
    unsigned char* WriteBufferPtr = WriteBuffer;
    //Get current data
    REGDUMPWORD newContext;
    //DISASM_INSTR newInstruction;
    DWORD newThreadId;
    const size_t memoryArrayCount = 32;
    duint newMemory[memoryArrayCount];
    duint newMemoryAddress[memoryArrayCount];
    duint oldMemory[memoryArrayCount];
    unsigned char newMemoryArrayCount = 0;
    DbgGetRegDumpEx((REGDUMP_AVX512*)&newContext.registers, sizeof(REGDUMP)); //TODO: Migrate
    newThreadId = ThreadGetId(hActiveThread);
    // Don't try to resolve memory values for invalid/lea/nop instructions
    if(newInstruction.Success() && !newInstruction.IsNop() && newInstruction.GetId() != ZYDIS_MNEMONIC_LEA)
    {
        // This can happen when trace execute instruction is used, we need to fix that or something would go wrong with the GUI.
        newContext.registers.regcontext.cip = (duint)newInstruction.Address();
        DISASM_ARGTYPE argType;
        duint value;
        unsigned char memoryContent[memoryContentSize];
        unsigned char memorySize;
        for(int i = 0; i < newInstruction.TotalOpCount(); i++)
        {
            memset(memoryContent, 0, sizeof(memoryContent));
            HandleZydisOperand(newInstruction, i, &argType, &value, memoryContent, &memorySize);
            // check for overflow of the memory buffer
            if(newMemoryArrayCount * sizeof(duint) + memorySize > memoryArrayCount * sizeof(duint))
                continue;
            // TODO: Support memory value of ??? for invalid memory access
            if(argType == arg_memory)
            {
                if(memorySize <= sizeof(duint))
                {
                    memcpy(&newMemory[newMemoryArrayCount], memoryContent, sizeof(duint));
                    newMemoryAddress[newMemoryArrayCount] = value;
                    newMemoryArrayCount++;
                }
                else
                    for(unsigned char index = 0; index < memorySize / sizeof(duint) + ((memorySize % sizeof(duint)) > 0 ? 1 : 0); index++)
                    {
                        memcpy(&newMemory[newMemoryArrayCount], memoryContent + sizeof(duint) * index, sizeof(duint));
                        newMemoryAddress[newMemoryArrayCount] = value + sizeof(duint) * index;
                        newMemoryArrayCount++;
                    }
            }
        }
        if(newInstruction.GetId() == ZYDIS_MNEMONIC_PUSH || newInstruction.GetId() == ZYDIS_MNEMONIC_PUSHF || newInstruction.GetId() == ZYDIS_MNEMONIC_PUSHFD
                || newInstruction.GetId() == ZYDIS_MNEMONIC_PUSHFQ || newInstruction.GetId() == ZYDIS_MNEMONIC_CALL //TODO: far call accesses 2 stack entries
          )
        {
            // Discussion: https://github.com/zyantific/zydis/issues/510
            MemRead(newContext.registers.regcontext.csp - sizeof(duint), &newMemory[newMemoryArrayCount], sizeof(duint));
            newMemoryAddress[--newMemoryArrayCount] = newContext.registers.regcontext.csp - sizeof(duint);
            newMemoryArrayCount++;
        }
        //TODO: PUSHAD
        assert(newMemoryArrayCount < memoryArrayCount);
    }
    if(rtPrevInstAvailable)
    {
        for(unsigned char i = 0; i < rtOldMemoryArrayCount; i++)
        {
            MemRead(rtOldMemoryAddress[i], oldMemory + i, sizeof(duint));
        }
        //Delta compress registers
        //Data layout is Structure of Arrays to gather the same type of data in continuous memory to improve RLE compression performance.
        //1byte:block type,1byte:reg changed count,1byte:memory accessed count,1byte:flags,4byte/none:threadid,string:opcode,1byte[]:position,ptrbyte[]:regvalue,1byte[]:flags,ptrbyte[]:address,ptrbyte[]:oldmem,ptrbyte[]:newmem

        //Always record state of LAST INSTRUCTION! (NOT current instruction)
        unsigned char changed = 0;
        for(unsigned char i = 0; i < _countof(rtOldContext.regword); i++)
        {
            //rtRecordedInstructions - 1 hack: always record full registers dump at first instruction (recorded at 2nd instruction execution time)
            //prints ASCII table in run trace at first instruction :)
            if(rtOldContext.regword[i] != newContext.regword[i] || rtOldContextChanged[i] || ((rtRecordedInstructions - 1) % MAX_INSTRUCTIONS_TRACED_FULL_REG_DUMP == 0))
                changed++;
        }
        unsigned char blockFlags = 0;
        if(newThreadId != rtOldThreadId || ((rtRecordedInstructions - 1) % MAX_INSTRUCTIONS_TRACED_FULL_REG_DUMP == 0))
            blockFlags = 0x80;
        blockFlags |= rtOldOpcodeSize;

        WriteBufferPtr[0] = 0; //1byte: block type
        WriteBufferPtr[1] = changed; //1byte: registers changed
        WriteBufferPtr[2] = rtOldMemoryArrayCount; //1byte: memory accesses count
        WriteBufferPtr[3] = blockFlags; //1byte: flags and opcode size
        WriteBufferPtr += 4;
        if(newThreadId != rtOldThreadId || rtNeedThreadId || ((rtRecordedInstructions - 1) % MAX_INSTRUCTIONS_TRACED_FULL_REG_DUMP == 0))
        {
            memcpy(WriteBufferPtr, &rtOldThreadId, sizeof(rtOldThreadId));
            WriteBufferPtr += sizeof(rtOldThreadId);
            rtNeedThreadId = (newThreadId != rtOldThreadId);
        }
        memcpy(WriteBufferPtr, rtOldOpcode, rtOldOpcodeSize);
        WriteBufferPtr += rtOldOpcodeSize;
        int lastChangedPosition = -1; //-1
        for(int i = 0; i < _countof(rtOldContext.regword); i++) //1byte: position
        {
            if(rtOldContext.regword[i] != newContext.regword[i] || rtOldContextChanged[i] || ((rtRecordedInstructions - 1) % MAX_INSTRUCTIONS_TRACED_FULL_REG_DUMP == 0))
            {
                WriteBufferPtr[0] = (unsigned char)(i - lastChangedPosition - 1);
                WriteBufferPtr++;
                lastChangedPosition = i;
            }
        }
        for(unsigned char i = 0; i < _countof(rtOldContext.regword); i++) //ptrbyte: newvalue
        {
            if(rtOldContext.regword[i] != newContext.regword[i] || rtOldContextChanged[i] || ((rtRecordedInstructions - 1) % MAX_INSTRUCTIONS_TRACED_FULL_REG_DUMP == 0))
            {
                memcpy(WriteBufferPtr, &rtOldContext.regword[i], sizeof(duint));
                WriteBufferPtr += sizeof(duint);
            }
            rtOldContextChanged[i] = (rtOldContext.regword[i] != newContext.regword[i]);
        }
        for(unsigned char i = 0; i < rtOldMemoryArrayCount; i++) //1byte: flags
        {
            unsigned char memoryOperandFlags = 0;
            if(rtOldMemory[i] == oldMemory[i]) //bit 0: memory is unchanged, no new memory is saved
                memoryOperandFlags |= 1;
            //proposed flags: is memory valid, is memory zero
            WriteBufferPtr[0] = memoryOperandFlags;
            WriteBufferPtr += 1;
        }
        for(unsigned char i = 0; i < rtOldMemoryArrayCount; i++) //ptrbyte: address
        {
            memcpy(WriteBufferPtr, &rtOldMemoryAddress[i], sizeof(duint));
            WriteBufferPtr += sizeof(duint);
        }
        for(unsigned char i = 0; i < rtOldMemoryArrayCount; i++) //ptrbyte: old content
        {
            memcpy(WriteBufferPtr, &rtOldMemory[i], sizeof(duint));
            WriteBufferPtr += sizeof(duint);
        }
        for(unsigned char i = 0; i < rtOldMemoryArrayCount; i++) //ptrbyte: new content
        {
            if(rtOldMemory[i] != oldMemory[i])
            {
                memcpy(WriteBufferPtr, &oldMemory[i], sizeof(duint));
                WriteBufferPtr += sizeof(duint);
            }
        }
    }
    //Switch context buffers
    rtOldThreadId = newThreadId;
    rtOldContext = newContext;
    rtOldMemoryArrayCount = newMemoryArrayCount;
    memcpy(rtOldMemory, newMemory, sizeof(newMemory));
    memcpy(rtOldMemoryAddress, newMemoryAddress, sizeof(newMemoryAddress));
    memset(rtOldOpcode, 0, 16);
    rtOldOpcodeSize = newInstruction.Size() & 0x0F;
    MemRead(newContext.registers.regcontext.cip, rtOldOpcode, rtOldOpcodeSize);
    //Write to file
    if(rtPrevInstAvailable)
    {
        if(WriteBufferPtr - WriteBuffer <= sizeof(WriteBuffer))
        {
            DWORD written;
            WriteFile(rtFile, WriteBuffer, (DWORD)(WriteBufferPtr - WriteBuffer), &written, NULL);
            if(written < DWORD(WriteBufferPtr - WriteBuffer)) //Disk full?
            {
                CloseHandle(rtFile);
                String error = stringformatinline(StringUtils::sprintf("{winerror@%x}", GetLastError()));
                dprintf(QT_TRANSLATE_NOOP("DBG", "Trace recording has stopped unexpectedly because WriteFile() failed. GetLastError() = %s.\n"), error.c_str());
                rtEnabled = false;
            }
        }
        else
            __debugbreak(); // Buffer overrun?
    }
    rtPrevInstAvailable = true;
    rtRecordedInstructions++;

    dbgtracebrowserneedsupdate();
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

bool TraceRecordManager::enableTraceRecording(bool enabled, const char* fileName)
{
    if(enabled)
    {
        if(rtEnabled)
            enableTraceRecording(false, NULL); //re-enable run trace
        if(!DbgIsDebugging())
            return false;
        rtFile = CreateFileW(StringUtils::Utf8ToUtf16(fileName).c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_DELETE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if(rtFile != INVALID_HANDLE_VALUE)
        {
            LARGE_INTEGER size;
            if(GetFileSizeEx(rtFile, &size))
            {
                if(size.QuadPart != 0)
                {
                    SetFilePointer(rtFile, 0, 0, FILE_END);
                }
                else //file is empty, write some file header
                {
                    //TRAC, SIZE, JSON header
                    json_t* root = json_object();
                    json_object_set_new(root, "ver", json_integer(1));
                    json_object_set_new(root, "arch", json_string(ArchValue("x86", "x64")));
                    json_object_set_new(root, "hashAlgorithm", json_string("murmurhash"));
                    json_object_set_new(root, "hash", json_hex(dbgfunctionsget()->DbGetHash()));
                    json_object_set_new(root, "compression", json_string(""));
                    char path[MAX_PATH];
                    ModPathFromAddr(dbgdebuggedbase(), path, MAX_PATH);
                    json_object_set_new(root, "path", json_string(path));
                    char* headerinfo;
                    headerinfo = json_dumps(root, JSON_COMPACT);
                    size_t headerinfosize = strlen(headerinfo);
                    LARGE_INTEGER header;
                    DWORD written;
                    uint8_t TRAC[4] = { 'T', 'R', 'A', 'C' };
                    memcpy(&header.LowPart, TRAC, sizeof(TRAC));
                    header.HighPart = (LONG)headerinfosize;
                    WriteFile(rtFile, &header, 8, &written, nullptr);
                    if(written < 8) //read-only?
                    {
                        CloseHandle(rtFile);
                        json_free(headerinfo);
                        json_decref(root);
                        dputs(QT_TRANSLATE_NOOP("DBG", "Trace recording failed to start because the file header cannot be written."));
                        return false;
                    }
                    WriteFile(rtFile, headerinfo, (DWORD)headerinfosize, &written, nullptr);
                    json_free(headerinfo);
                    json_decref(root);
                    if(written < headerinfosize) //disk-full?
                    {
                        CloseHandle(rtFile);
                        dputs(QT_TRANSLATE_NOOP("DBG", "Trace recording failed to start because the file header cannot be written."));
                        return false;
                    }
                }
            }
            rtPrevInstAvailable = false;
            rtEnabled = true;
            rtRecordedInstructions = 0;
            rtNeedThreadId = true;
            for(size_t i = 0; i < _countof(rtOldContextChanged); i++)
                rtOldContextChanged[i] = true;
            dprintf(QT_TRANSLATE_NOOP("DBG", "Started trace recording to file: %s\n"), fileName);
            Zydis zydis;
            unsigned char instr[MAX_DISASM_BUFFER];
            auto cip = GetContextDataEx(hActiveThread, UE_CIP);
            if(MemRead(cip, instr, MAX_DISASM_BUFFER))
            {
                zydis.DisassembleSafe(cip, instr, MAX_DISASM_BUFFER);
                TraceExecuteRecord(zydis);
            }
            GuiOpenTraceFile(fileName);
            return true;
        }
        else
        {
            String error = stringformatinline(StringUtils::sprintf("{winerror@%x}", GetLastError()));
            dprintf(QT_TRANSLATE_NOOP("DBG", "Cannot create trace recording file. GetLastError() = %s.\n"), error.c_str());
            return false;
        }
    }
    else
    {
        if(rtEnabled)
        {
            CloseHandle(rtFile);
            rtPrevInstAvailable = false;
            rtEnabled = false;
            dputs(QT_TRANSLATE_NOOP("DBG", "Trace recording stopped."));
        }
        return true;
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
                TraceRecord.emplace(key, currentPage);
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

bool TraceRecordManager::isTraceRecordingEnabled()
{
    return rtEnabled;
}

void dbgtraceexecute(duint CIP)
{
    if(TraceRecord.getTraceRecordType(CIP) != TraceRecordManager::TraceRecordType::TraceRecordNone)
    {
        Zydis instruction;
        unsigned char data[MAX_DISASM_BUFFER];
        if(MemRead(CIP, data, MAX_DISASM_BUFFER))
        {
            instruction.DisassembleSafe(CIP, data, MAX_DISASM_BUFFER);
            if(TraceRecord.isTraceRecordingEnabled())
            {
                TraceRecord.TraceExecute(CIP, instruction.Size());
                TraceRecord.TraceExecuteRecord(instruction);
            }
            else
            {
                TraceRecord.TraceExecute(CIP, instruction.Size());
            }
        }
    }
    else
    {
        if(TraceRecord.isTraceRecordingEnabled())
        {
            Zydis instruction;
            unsigned char data[MAX_DISASM_BUFFER];
            if(MemRead(CIP, data, MAX_DISASM_BUFFER))
            {
                instruction.DisassembleSafe(CIP, data, MAX_DISASM_BUFFER);
                TraceRecord.TraceExecuteRecord(instruction);
            }
        }
    }
    TraceRecord.increaseInstructionCounter();
}
