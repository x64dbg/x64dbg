#include "QBeaEngine.h"
#include "StringUtil.h"
#include "EncodeMap.h"
#include "CodeFolding.h"

QBeaEngine::QBeaEngine(int maxModuleSize)
    : _tokenizer(maxModuleSize), mCodeFoldingManager(nullptr), _bLongDataInst(false)
{
    CapstoneTokenizer::UpdateColors();
    UpdateDataInstructionMap();
    this->mEncodeMap = new EncodeMap();
}

QBeaEngine::~QBeaEngine()
{
    delete this->mEncodeMap;
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
ulong QBeaEngine::DisassembleBack(byte_t* data, duint base, duint size, duint ip, int n)
{
    int i;
    uint abuf[128], addr, back, cmdsize;
    unsigned char* pdata;

    // Reset Disasm Structure
    Zydis cp;

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
            if(!cp.DisassembleSafe(addr + base, pdata, (int)size))
                cmdsize = 2; //heuristic for better output (FF FE or FE FF are usually part of an instruction)
            else
                cmdsize = cp.Size();

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
ulong QBeaEngine::DisassembleNext(byte_t* data, duint base, duint size, duint ip, int n)
{
    int i;
    uint cmdsize;
    unsigned char* pdata;

    // Reset Disasm Structure
    Zydis cp;

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
            if(!cp.DisassembleSafe(ip + base, pdata, (int)size))
                cmdsize = 1;
            else
                cmdsize = cp.Size();

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
Instruction_t QBeaEngine::DisassembleAt(byte_t* data, duint size, duint origBase, duint origInstRVA, bool datainstr)
{
    if(datainstr)
    {
        ENCODETYPE type = mEncodeMap->getDataType(origBase + origInstRVA);
        if(!mEncodeMap->isCode(type))
            return DecodeDataAt(data, size, origBase, origInstRVA, type);
    }
    //tokenize
    CapstoneTokenizer::InstructionToken cap;
    _tokenizer.Tokenize(origBase + origInstRVA, data, size, cap);
    int len = _tokenizer.Size();

    const auto & cp = _tokenizer.GetCapstone();
    bool success = cp.Success();


    auto branchType = Instruction_t::None;
    Instruction_t wInst;
    if(success && cp.IsBranchType(Zydis::BTJmp | Zydis::BTCall | Zydis::BTRet | Zydis::BTLoop))
    {
        wInst.branchDestination = DbgGetBranchDestination(origBase + origInstRVA);
        if(cp.IsBranchType(Zydis::BTUncondJmp))
            branchType = Instruction_t::Unconditional;
        else if(cp.IsBranchType(Zydis::BTCall))
            branchType = Instruction_t::Call;
        else if(cp.IsBranchType(Zydis::BTCondJmp))
            branchType = Instruction_t::Conditional;
    }
    else
        wInst.branchDestination = 0;

    wInst.instStr = QString(cp.InstructionText().c_str());
    wInst.dump = QByteArray((const char*)data, len);
    wInst.rva = origInstRVA;
    if(mCodeFoldingManager && mCodeFoldingManager->isFolded(origInstRVA))
        wInst.length = mCodeFoldingManager->getFoldEnd(origInstRVA + origBase) - (origInstRVA + origBase) + 1;
    else
        wInst.length = len;
    wInst.branchType = branchType;
    wInst.tokens = cap;
    cp.BytesGroup(&wInst.prefixSize, &wInst.opcodeSize, &wInst.group1Size, &wInst.group2Size, &wInst.group3Size);

    if(!success)
        return wInst;

    auto instr = cp.GetInstr();
    cp.RegInfo(reginfo);

    for(size_t i = 0; i < _countof(instr->accessedFlags); ++i)
    {
        auto flagAction = instr->accessedFlags[i].action;
        if(flagAction == ZYDIS_CPUFLAG_ACTION_NONE)
            continue;

        Zydis::RegAccessInfo rai;
        switch(flagAction)
        {
        case ZYDIS_CPUFLAG_ACTION_MODIFIED:
        case ZYDIS_CPUFLAG_ACTION_SET_0:
        case ZYDIS_CPUFLAG_ACTION_SET_1:
            rai = Zydis::RAIWrite;
            break;
        case ZYDIS_CPUFLAG_ACTION_TESTED:
            rai = Zydis::RAIRead;
            break;
        default:
            rai = Zydis::RAINone;
            break;
        }

        reginfo[ZYDIS_REGISTER_RFLAGS] = Zydis::RAINone;
        reginfo[ZYDIS_REGISTER_EFLAGS] = Zydis::RAINone;
        reginfo[ZYDIS_REGISTER_FLAGS]  = Zydis::RAINone;

        wInst.regsReferenced.emplace_back(cp.FlagName(ZydisCPUFlag(i)), rai);
    }

    reginfo[ArchValue(ZYDIS_REGISTER_EIP, ZYDIS_REGISTER_RIP)] = Zydis::RAINone;
    for(int i = ZYDIS_REGISTER_NONE; i <= ZYDIS_REGISTER_MAX_VALUE; ++i)
        if(reginfo[i])
            wInst.regsReferenced.emplace_back(cp.RegName(ZydisRegister(i)), reginfo[i]);

    return wInst;
}

Instruction_t QBeaEngine::DecodeDataAt(byte_t* data, duint size, duint origBase, duint origInstRVA, ENCODETYPE type)
{
    //tokenize
    CapstoneTokenizer::InstructionToken cap;

    auto infoIter = dataInstMap.find(type);
    if(infoIter == dataInstMap.end())
        infoIter = dataInstMap.find(enc_byte);

    int len = mEncodeMap->getDataSize(origBase + origInstRVA, 1);

    QString mnemonic = _bLongDataInst ? infoIter.value().longName : infoIter.value().shortName;

    len = std::min(len, (int)size);

    QString datastr = GetDataTypeString(data, len, type);

    _tokenizer.TokenizeData(mnemonic, datastr, cap);

    Instruction_t wInst;
    wInst.instStr = mnemonic + " " + datastr;
    wInst.dump = QByteArray((const char*)data, len);
    wInst.rva = origInstRVA;
    wInst.length = len;
    wInst.branchType = Instruction_t::None;
    wInst.branchDestination = 0;
    wInst.tokens = cap;

    return wInst;
}

void QBeaEngine::UpdateDataInstructionMap()
{
    dataInstMap.clear();
    dataInstMap.insert(enc_byte, {"db", "byte", "int8"});
    dataInstMap.insert(enc_word, {"dw", "word", "short"});
    dataInstMap.insert(enc_dword, {"dd", "dword", "int"});
    dataInstMap.insert(enc_fword, {"df", "fword", "fword"});
    dataInstMap.insert(enc_qword, {"dq", "qword", "long"});
    dataInstMap.insert(enc_tbyte, {"tbyte", "tbyte", "tbyte"});
    dataInstMap.insert(enc_oword, {"oword", "oword", "oword"});
    dataInstMap.insert(enc_mmword, {"mmword", "mmword", "long long"});
    dataInstMap.insert(enc_xmmword, {"xmmword", "xmmword", "_m128"});
    dataInstMap.insert(enc_ymmword, {"ymmword", "ymmword", "_m256"});
    dataInstMap.insert(enc_real4, {"real4", "real4", "float"});
    dataInstMap.insert(enc_real8, {"real8", "real8", "double"});
    dataInstMap.insert(enc_real10, {"real10", "real10", "long double"});
    dataInstMap.insert(enc_ascii, {"ascii", "ascii", "string"});
    dataInstMap.insert(enc_unicode, {"unicode", "unicode", "wstring"});
}

void QBeaEngine::setCodeFoldingManager(CodeFoldingHelper* CodeFoldingManager)
{
    mCodeFoldingManager = CodeFoldingManager;
}

void QBeaEngine::UpdateConfig()
{
    _bLongDataInst = ConfigBool("Disassembler", "LongDataInstruction");
    _tokenizer.UpdateConfig();
}

void formatOpcodeString(const Instruction_t & inst, RichTextPainter::List & list)
{
    RichTextPainter::CustomRichText_t curByte;
    assert(list.empty()); //List must be empty before use
    curByte.highlightColor = ConfigColor("DisassemblyRelocationUnderlineColor");
    curByte.highlightWidth = 1;
    curByte.flags = RichTextPainter::FlagAll;
    curByte.highlight = false;
    for(int i = 0; i < inst.dump.size(); i++)
    {
        curByte.text = ToByteString(inst.dump.at(i));
        list.push_back(curByte);
    }
    if(inst.prefixSize > 0)
    {
        list.at(inst.prefixSize - 1).text.append(':');
    }
    list.at(inst.opcodeSize + inst.prefixSize - 1).text.append(' ');
    if(inst.group1Size > 0)
    {
        list.at(inst.opcodeSize + inst.prefixSize + inst.group1Size - 1).text.append(' ');
    }
    if(inst.group2Size > 0)
    {
        list.at(inst.opcodeSize + inst.prefixSize + inst.group1Size + inst.group2Size - 1).text.append(' ');
    }
    /*if(inst.group3Size > 0)
    {
        output.insert((inst.opcodeSize + inst.prefixSize + inst.group1Size + inst.group2Size) * 2 + offset, '?');
    }
    output += QString("|%1.%2.%3.%4").arg(inst.opcodeSize).arg(inst.group1Size).arg(inst.group2Size).arg(inst.group3Size);
    */
}
