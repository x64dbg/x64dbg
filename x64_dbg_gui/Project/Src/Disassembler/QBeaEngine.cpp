#include "QBeaEngine.h"

QBeaEngine::QBeaEngine()
{
    // Reset the Disasm structure
    memset(&mDisasmStruct, 0, sizeof(DISASM));
    BeaTokenizer::Init();
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
ulong QBeaEngine::DisassembleBack(byte_t* data, uint_t base, uint_t size, uint_t ip, int n)
{

    const unsigned int max_instructions = 128;

    Q_UNUSED(base);
    int i;
    uint_t abuf[131], addr, back, cmdsize;
    byte_t* pdata;
    int len;

    // Reset Disasm Structure
    memset(&mDisasmStruct, 0, sizeof(DISASM));
#ifdef _WIN64
    mDisasmStruct.Archi = 64;
#endif
    mDisasmStruct.Options = NoformatNumeral;

    // Check if the pointer is not null
    if(data == NULL)
        return 0;

    // Round the number of back instructions to 127
    if(n < 0)
        n = 0;
    else if(n >= max_instructions)
        n = max_instructions - 1;

    // Check if the instruction pointer ip is not outside the memory range
    if(ip >= size)
        ip = size - 1;

    // Obvious answer
    if(n == 0)
        return ip;

    if(ip < (uint_t)n)
        return ip;

    back = 16 * (n + 3); // Instruction length limited to 16

    if(ip < back)
        back = ip;

    addr = ip - back;

    pdata = data + addr;

    for(i = 0; addr < ip; i++)
    {
        abuf[i % max_instructions] = addr;

        mDisasmStruct.EIP = (UIntPtr)pdata;
        len = Disasm(&mDisasmStruct);
        cmdsize = (len < 1) ? 1 : len ;

        pdata += cmdsize;
        addr += cmdsize;
        back -= cmdsize;
    }

    if(i < n)
        return abuf[0];
    else
        return abuf[(i - n + max_instructions) % max_instructions];
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
ulong QBeaEngine::DisassembleNext(byte_t* data, uint_t base, uint_t size, uint_t ip, int n)
{
    Q_UNUSED(base);
    int i;
    uint_t cmdsize;
    byte_t* pdata;
    int len;

    // Reset Disasm Structure
    memset(&mDisasmStruct, 0, sizeof(DISASM));
#ifdef _WIN64
    mDisasmStruct.Archi = 64;
#endif
    mDisasmStruct.Options = NoformatNumeral;


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
        mDisasmStruct.EIP = (UIntPtr)pdata;
        mDisasmStruct.SecurityBlock = (UIntPtr)size;
        len = Disasm(&mDisasmStruct);
        cmdsize = (len < 1) ? 1 : len;

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
Instruction_t QBeaEngine::DisassembleAt(byte_t* data, uint_t size, uint_t instIndex, uint_t origBase, uint_t origInstRVA)
{
    Instruction_t wInst;
    int len;

    // Reset Disasm Structure
    memset(&mDisasmStruct, 0, sizeof(DISASM));

#ifdef _WIN64
    mDisasmStruct.Archi = 64;
#endif
    mDisasmStruct.Options = NoformatNumeral | ShowSegmentRegs;

    mDisasmStruct.EIP = (UIntPtr)((uint_t)data + (uint_t)instIndex);
    mDisasmStruct.VirtualAddr = origBase + origInstRVA;
    mDisasmStruct.SecurityBlock = (UIntPtr)((uint_t)size - instIndex);

    len = Disasm(&mDisasmStruct);
    len = (len < 1) ? 1 : len ;

    wInst.instStr = QString(mDisasmStruct.CompleteInstr);
    int instrLen = wInst.instStr.length();
    if(instrLen && wInst.instStr.at(instrLen - 1) == ' ')
        wInst.instStr.chop(1);
    wInst.dump = QByteArray((char*)mDisasmStruct.EIP, len);
    wInst.rva = origInstRVA;
    wInst.length = len;
    wInst.disasm = mDisasmStruct;

    //tokenize
    BeaTokenizer::TokenizeInstruction(&wInst.tokens, &mDisasmStruct);

    return wInst;
}
