#include "disasm_helper.h"
#include "BeaEngine\BeaEngine.h"

const char* disasmtext(uint addr)
{
    unsigned char buffer[16]="";
    DbgMemRead(addr, buffer, 16);
    DISASM disasm;
    disasm.VirtualAddr=addr;
    disasm.EIP=(UIntPtr)buffer;
    int len=Disasm(&disasm);
    static char instruction[INSTRUCT_LENGTH]="";
    if(len==UNKNOWN_OPCODE)
        strcpy(instruction, "???");
    else
        strcpy(instruction, disasm.CompleteInstr);
    return instruction;
}
