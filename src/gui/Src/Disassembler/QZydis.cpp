#include "QZydis.h"
#include "StringUtil.h"
#include <Utils/EncodeMap.h>
#include <Utils/CodeFolding.h>
#include <Bridge.h>

#ifndef _countof
#define _countof(array) (sizeof(array) / sizeof(array[0]))
#endif // _countof

QZydis::QZydis(int maxModuleSize, Architecture* architecture)
    : mTokenizer(maxModuleSize, architecture), mArchitecture(architecture)
{
    ZydisTokenizer::UpdateColors();
    UpdateDataInstructionMap();
    mEncodeMap = new EncodeMap();
}

QZydis::~QZydis()
{
    delete mEncodeMap;
}

/**
 * @brief       Return the address of the nth instruction before the instruction pointed by ip.                 @n
 *              This function has been grabbed from OllyDbg ("Disassembleback" in asmserv.c)
 *
 * @param[in]   data    Address of the data to disassemble
 * @param[in]   base    Original base address of the memory page (Required to disassemble destination addresses)
 * @param[in]   size    Size of the data block pointed by data
 * @param[in]   ip      RVA of the current instruction (Relative to data pointer)
 * @param[in]   n       Number of instruction back
 *
 * @return      Return the RVA (Relative to the data pointer) of the nth instruction before the instruction pointed by ip
 */
ulong QZydis::DisassembleBack(const uint8_t* data, duint base, duint size, duint ip, int n)
{
    int i;
    uint abuf[128], addr, back, cmdsize;
    const unsigned char* pdata;

    // Reset Disasm Structure
    Zydis zydis(mArchitecture->disasm64());

    // Check if the pointer is not null
    if(data == NULL)
        return 0;

    // Round the number of back instructions to 127
    if(n < 0)
        n = 0;
    else if(n > 127)
        n = 127;

    // Check if the instruction pointer ip is not outside the memory range
    if(ip >= size)
        ip = size - 1;

    // Obvious answer
    if(n == 0)
        return ip;

    if(ip < (uint)n)
        return ip;

    //TODO: buffer overflow due to unchecked "back" value
    back = MAX_DISASM_BUFFER * (n + 3); // Instruction length limited to 16

    if(ip < back)
        back = ip;

    addr = ip - back;
    if(mCodeFoldingManager && mCodeFoldingManager->isFolded(addr + base))
    {
        duint newback = mCodeFoldingManager->getFoldBegin(addr + base);
        if(newback >= base && newback < size + base)
            addr = newback - base;
    }

    pdata = data + addr;

    for(i = 0; addr < ip; i++)
    {
        abuf[i % 128] = addr;

        if(mCodeFoldingManager && mCodeFoldingManager->isFolded(addr + base))
        {
            duint newaddr = mCodeFoldingManager->getFoldBegin(addr + base);
            if(newaddr >= base)
            {
                addr = newaddr - base;
            }
            cmdsize = mCodeFoldingManager->getFoldEnd(addr + base) - (addr + base) + 1;
        }
        else
        {
            if(!zydis.DisassembleSafe(addr + base, pdata, (int)size))
                cmdsize = 2; //heuristic for better output (FF FE or FE FF are usually part of an instruction)
            else
                cmdsize = zydis.Size();

            cmdsize = mEncodeMap->getDataSize(base + addr, cmdsize);

        }


        pdata += cmdsize;
        addr += cmdsize;
        back -= cmdsize;
        size -= cmdsize;
    }

    if(i < n)
        return abuf[0];
    else
        return abuf[(i - n + 128) % 128];

}

/**
 * @brief       Return the address of the nth instruction after the instruction pointed by ip.                 @n
 *              This function has been grabbed from OllyDbg ("Disassembleforward" in asmserv.c)
 *
 * @param[in]   data    Address of the data to disassemble
 * @param[in]   base    Original base address of the memory page (Required to disassemble destination addresses)
 * @param[in]   size    Size of the data block pointed by data
 * @param[in]   ip      RVA of the current instruction (Relative to data pointer)
 * @param[in]   n       Number of instruction next
 *
 * @return      Return the RVA (Relative to the data pointer) of the nth instruction after the instruction pointed by ip
 */
ulong QZydis::DisassembleNext(const uint8_t* data, duint base, duint size, duint ip, int n)
{
    int i;
    uint cmdsize;
    const unsigned char* pdata;

    // Reset Disasm Structure
    Zydis zydis(mArchitecture->disasm64());

    if(data == NULL)
        return 0;

    if(ip >= size)
        ip = size - 1;

    if(n <= 0)
        return ip;


    pdata = data + ip;
    size -= ip;

    for(i = 0; i < n && size > 0; i++)
    {
        if(mCodeFoldingManager && mCodeFoldingManager->isFolded(ip + base))
        {
            cmdsize = mCodeFoldingManager->getFoldEnd(ip + base) - (ip + base) + 1;
        }
        else
        {
            if(!zydis.DisassembleSafe(ip + base, pdata, (int)size))
                cmdsize = 1;
            else
                cmdsize = zydis.Size();

            cmdsize = mEncodeMap->getDataSize(base + ip, cmdsize);

        }

        pdata += cmdsize;
        ip += cmdsize;
        size -= cmdsize;
    }

    return ip;
}

/**
 * @brief       Disassemble the instruction at the given ip RVA.
 *
 * @param[in]   data            Pointer to memory data (Can be either a buffer or the original data memory)
 * @param[in]   size            Size of the memory pointed by data (Can be the memory page size if data points to the original memory page base address)
 * @param[in]   origBase        Original base address of the memory page (Required to disassemble destination addresses)
 * @param[in]   origInstRVA     Original Instruction RVA of the instruction to disassemble
 *
 * @return      Return the disassembled instruction
 */
Instruction_t QZydis::DisassembleAt(const uint8_t* data, duint size, duint origBase, duint origInstRVA, bool datainstr)
{
    if(datainstr)
    {
        ENCODETYPE type = mEncodeMap->getDataType(origBase + origInstRVA);
        if(!mEncodeMap->isCode(type))
            return DecodeDataAt(data, size, origBase, origInstRVA, type);
    }

    //tokenize
    ZydisTokenizer::InstructionToken cap;
    mTokenizer.Tokenize(origBase + origInstRVA, data, size, cap);
    int len = mTokenizer.Size();

    const auto & zydis = mTokenizer.GetZydis();
    bool success = zydis.Success();

    auto branchType = Instruction_t::None;
    Instruction_t inst;
    if(success && zydis.IsBranchType(Zydis::BTJmp | Zydis::BTCall | Zydis::BTRet | Zydis::BTLoop | Zydis::BTXbegin))
    {
        inst.branchDestination = DbgGetBranchDestination(origBase + origInstRVA);
        if(zydis.IsBranchType(Zydis::BTUncondJmp))
            branchType = Instruction_t::Unconditional;
        else if(zydis.IsBranchType(Zydis::BTCall))
            branchType = Instruction_t::Call;
        else if(zydis.IsBranchType(Zydis::BTCondJmp) || zydis.IsBranchType(Zydis::BTLoop))
            branchType = Instruction_t::Conditional;
    }
    else
        inst.branchDestination = 0;

    inst.instStr = QString(zydis.InstructionText().c_str());
    inst.dump = QByteArray((const char*)data, len);
    inst.rva = origInstRVA;
    if(mCodeFoldingManager && mCodeFoldingManager->isFolded(origInstRVA))
        inst.length = mCodeFoldingManager->getFoldEnd(origInstRVA + origBase) - (origInstRVA + origBase) + 1;
    else
        inst.length = len;
    inst.branchType = branchType;
    inst.tokens = cap;
    zydis.BytesGroup(&inst.prefixSize, &inst.opcodeSize, &inst.group1Size, &inst.group2Size, &inst.group3Size);
    for(uint8_t i = 0; i < _countof(inst.vectorElementType); ++i)
        inst.vectorElementType[i] = zydis.getVectorElementType(i);

    if(!success)
        return inst;

    uint8_t regInfo[ZYDIS_REGISTER_MAX_VALUE + 1];
    uint8_t flagInfo[32];
    zydis.RegInfo(regInfo);
    zydis.FlagInfo(flagInfo);

    regInfo[ZYDIS_REGISTER_RFLAGS] = Zydis::RAINone;
    regInfo[ZYDIS_REGISTER_EFLAGS] = Zydis::RAINone;
    regInfo[ZYDIS_REGISTER_FLAGS]  = Zydis::RAINone;
    regInfo[mArchitecture->disasm64() ? ZYDIS_REGISTER_RIP : ZYDIS_REGISTER_EIP] = Zydis::RAINone;

    inst.regsReferenced.reserve(ZYDIS_REGISTER_MAX_VALUE + 21);
    for(int i = ZYDIS_REGISTER_NONE; i <= ZYDIS_REGISTER_MAX_VALUE; ++i)
    {
        if(regInfo[i] != Zydis::RAINone)
        {
            inst.regsReferenced.emplace_back(zydis.RegName(ZydisRegister(i)), regInfo[i]);
        }
    }
    for(uint8_t i = 0; i < _countof(flagInfo); i++)
    {
        auto flag = 1u << i;
        auto name = zydis.FlagName(flag);
        auto rai = flagInfo[i];
        if(name != nullptr && rai != Zydis::RAINone)
        {
            inst.regsReferenced.emplace_back(name, rai);
        }
    }

    // Info about volatile and nonvolatile registers
    if(zydis.IsBranchType(Zydis::BranchType::BTCall))
    {
        enum : uint8_t
        {
            Volatile = Zydis::RAIImplicit | Zydis::RAIWrite,
            Parameter = Volatile | Zydis::RAIRead,
        };
#define info(reg, type) inst.regsReferenced.emplace_back(#reg, type)

        if(mArchitecture->disasm64())
        {
            // https://docs.microsoft.com/en-us/cpp/build/x64-software-conventions
            info(rax, Volatile);
            info(rcx, Parameter);
            info(rdx, Parameter);
            info(r8, Parameter);
            info(r9, Parameter);
            info(r10, Volatile);
            info(r11, Volatile);
            info(xmm0, Parameter);
            info(ymm0, Parameter);
            info(xmm1, Parameter);
            info(ymm1, Parameter);
            info(xmm2, Parameter);
            info(ymm2, Parameter);
            info(xmm3, Parameter);
            info(ymm3, Parameter);
            info(xmm4, Parameter);
            info(ymm4, Parameter);
            info(xmm5, Parameter);
            info(ymm5, Parameter);
        }
        else
        {
            // https://en.wikipedia.org/wiki/X86_calling_conventions#Caller-saved_(volatile)_registers
            info(eax, Volatile);
            info(edx, Volatile);
            info(ecx, Volatile);
        }

#undef info
    }

    return inst;
}

Instruction_t QZydis::DecodeDataAt(const uint8_t* data, duint size, duint origBase, duint origInstRVA, ENCODETYPE type)
{
    //tokenize
    ZydisTokenizer::InstructionToken cap;

    auto infoIter = mDataInstMap.find(type);
    if(infoIter == mDataInstMap.end())
        infoIter = mDataInstMap.find(enc_byte);

    int len = mEncodeMap->getDataSize(origBase + origInstRVA, 1);

    QString mnemonic = mLongDataInst ? infoIter.value().longName : infoIter.value().shortName;

    len = std::min(len, (int)size);

    QString datastr = GetDataTypeString(data, len, type);

    mTokenizer.TokenizeData(mnemonic, datastr, cap);

    Instruction_t inst;
    inst.instStr = mnemonic + " " + datastr;
    inst.dump = QByteArray((const char*)data, len);
    inst.rva = origInstRVA;
    inst.length = len;
    inst.branchType = Instruction_t::None;
    inst.branchDestination = 0;
    inst.tokens = cap;
    inst.prefixSize = 0;
    inst.opcodeSize = len;
    inst.group1Size = 0;
    inst.group2Size = 0;
    inst.group3Size = 0;
    inst.vectorElementType[0] = Zydis::VETDefault;
    inst.vectorElementType[1] = Zydis::VETDefault;
    inst.vectorElementType[2] = Zydis::VETDefault;
    inst.vectorElementType[3] = Zydis::VETDefault;

    return inst;
}

void QZydis::UpdateDataInstructionMap()
{
    mDataInstMap.clear();
    mDataInstMap.insert(enc_byte, {"db", "byte", "int8"});
    mDataInstMap.insert(enc_word, {"dw", "word", "short"});
    mDataInstMap.insert(enc_dword, {"dd", "dword", "int"});
    mDataInstMap.insert(enc_fword, {"df", "fword", "fword"});
    mDataInstMap.insert(enc_qword, {"dq", "qword", "long"});
    mDataInstMap.insert(enc_tbyte, {"tbyte", "tbyte", "tbyte"});
    mDataInstMap.insert(enc_oword, {"oword", "oword", "oword"});
    mDataInstMap.insert(enc_mmword, {"mmword", "mmword", "long long"});
    mDataInstMap.insert(enc_xmmword, {"xmmword", "xmmword", "_m128"});
    mDataInstMap.insert(enc_ymmword, {"ymmword", "ymmword", "_m256"});
    mDataInstMap.insert(enc_zmmword, {"zmmword", "zmmword", "_m512"});
    mDataInstMap.insert(enc_real4, {"real4", "real4", "float"});
    mDataInstMap.insert(enc_real8, {"real8", "real8", "double"});
    mDataInstMap.insert(enc_real10, {"real10", "real10", "long double"});
    mDataInstMap.insert(enc_ascii, {"ascii", "ascii", "string"});
    mDataInstMap.insert(enc_unicode, {"unicode", "unicode", "wstring"});
}

void QZydis::setCodeFoldingManager(CodeFoldingHelper* CodeFoldingManager)
{
    mCodeFoldingManager = CodeFoldingManager;
}

void QZydis::UpdateConfig()
{
    mLongDataInst = ConfigBool("Disassembler", "LongDataInstruction");
    mTokenizer.UpdateConfig();
}

void QZydis::UpdateArchitecture()
{
    mTokenizer.UpdateArchitecture();
}

void formatOpcodeString(const Instruction_t & inst, RichTextPainter::List & list, std::vector<std::pair<size_t, bool>> & realBytes)
{
    RichTextPainter::CustomRichText_t curByte;
    auto size = inst.dump.size();
    assert(list.empty()); //List must be empty before use
    curByte.underlineWidth = 1;
    curByte.flags = RichTextPainter::FlagAll;
    curByte.underline = false;
    list.reserve(size + 5);
    realBytes.reserve(size + 5);
    for(int i = 0; i < size; i++)
    {
        curByte.text = ToByteString(inst.dump.at(i));
        list.push_back(curByte);
        realBytes.push_back({i, true});

        auto addCh = [&](char ch)
        {
            curByte.text = QString(ch);
            list.push_back(curByte);
            realBytes.push_back({i, false});
        };

        if(inst.prefixSize && i + 1 == inst.prefixSize)
            addCh(':');
        else if(inst.opcodeSize && i + 1 == inst.prefixSize + inst.opcodeSize)
            addCh(' ');
        else if(inst.group1Size && i + 1 == inst.prefixSize + inst.opcodeSize + inst.group1Size)
            addCh(' ');
        else if(inst.group2Size && i + 1 == inst.prefixSize + inst.opcodeSize + inst.group1Size + inst.group2Size)
            addCh(' ');
        else if(inst.group3Size && i + 1 == inst.prefixSize + inst.opcodeSize + inst.group1Size + inst.group2Size + inst.group3Size)
            addCh(' ');

    }
}
