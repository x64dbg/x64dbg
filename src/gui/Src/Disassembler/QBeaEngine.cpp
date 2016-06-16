#include "QBeaEngine.h"
#include "StringUtil.h"

QBeaEngine::QBeaEngine(int maxModuleSize)
    : _tokenizer(maxModuleSize), mCIP(0)
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
    Q_UNUSED(base)
    int i;
    uint abuf[131], addr, back, cmdsize;
    unsigned char* pdata;

    // Reset Disasm Structure
    Capstone cp;

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

    back = MAX_DISASM_BUFFER * (n + 3); // Instruction length limited to 16

    if(ip < back)
        back = ip;

    addr = ip - back;

    pdata = data + addr;

    for(i = 0; addr < ip; i++)
    {
        abuf[i % 128] = addr;

        if(!cp.Disassemble(0, pdata, (int)size))
            cmdsize = 2; //heuristic for better output (FF FE or FE FF are usually part of an instruction)
        else
            cmdsize = cp.Size();

        cmdsize = mEncodeMap->getDataSize(base + addr, cmdsize, mCIP); //If CIP at current address, it must be code, otherwise try decode as data

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
    Q_UNUSED(base)
    int i;
    uint cmdsize;
    unsigned char* pdata;

    // Reset Disasm Structure
    Capstone cp;

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
        if(!cp.Disassemble(0, pdata, (int)size))
            cmdsize = 1;
        else
            cmdsize = cp.Size();

        cmdsize = mEncodeMap->getDataSize(base + ip, cmdsize, mCIP); //If CIP at current address, it must be code, otherwise try decode as data

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
 * @param[in]   instIndex       Offset to reach the instruction data from the data pointer
 * @param[in]   origBase        Original base address of the memory page (Required to disassemble destination addresses)
 * @param[in]   origInstRVA     Original Instruction RVA of the instruction to disassemble
 *
 * @return      Return the disassembled instruction
 */

Instruction_t QBeaEngine::DisassembleAt(byte_t* data, duint size, duint instIndex, duint origBase, duint origInstRVA)
{
    //tokenize
    CapstoneTokenizer::InstructionToken cap;
    _tokenizer.Tokenize(origBase + origInstRVA, data, size, cap);
    int len = _tokenizer.Size();

    const auto & cp = _tokenizer.GetCapstone();
    bool success = cp.Success();


    ENCODETYPE type = enc_code;

    type = mEncodeMap->getDataType(origBase + origInstRVA, cp.Success() ? len : 0, mCIP);

    if(type != enc_unknown && type != enc_code && type != enc_middle)
        return DecodeDataAt(data, size, instIndex, origBase, origInstRVA, type);

    auto branchType = Instruction_t::None;
    if(success && (cp.InGroup(CS_GRP_JUMP) || cp.IsLoop()))
    {
        switch(cp.GetId())
        {
        case X86_INS_JMP:
            branchType = Instruction_t::Unconditional;
            break;
        default:
            branchType = Instruction_t::Conditional;
            break;
        }
    }

    Instruction_t wInst;
    wInst.instStr = QString(cp.InstructionText().c_str());
    wInst.dump = QByteArray((const char*)data, len);
    wInst.rva = origInstRVA;
    wInst.length = len;
    wInst.branchType = branchType;
    wInst.branchDestination = cp.BranchDestination();
    wInst.tokens = cap;

    return wInst;
}



Instruction_t QBeaEngine::DecodeDataAt(byte_t* data, duint size, duint instIndex, duint origBase, duint origInstRVA, ENCODETYPE type)
{
    //tokenize
    CapstoneTokenizer::InstructionToken cap;

    DataInstructionInfo info;

    auto & infoIter = dataInstMap.find(type);

    if(infoIter == dataInstMap.end())
    {
        infoIter = dataInstMap.find(enc_byte);
    }


    int len = mEncodeMap->getDataSize(origBase + origInstRVA, 0, mCIP);

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



void QBeaEngine::UpdateConfig()
{
    _bLongDataInst = ConfigBool("Disassembler", "LongDataInstruction");
    _tokenizer.UpdateConfig();
}
