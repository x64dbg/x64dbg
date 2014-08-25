#include "StackEmulator.h"


namespace fa
{
#define _isPush(disasm)  ((strcmp((disasm)->Instruction.Mnemonic ,"push ") == 0) )
#define _isPop(disasm)  ((strcmp((disasm)->Instruction.Mnemonic ,"pop ") == 0) )
#define _isSub(disasm)  ((strcmp((disasm)->Instruction.Mnemonic ,"sub ") == 0) )
#define _isAdd(disasm)  ((strcmp((disasm)->Instruction.Mnemonic ,"add ") == 0) )




StackEmulator::StackEmulator(void) : mStackpointer(0)
{
    for(unsigned int i = 0; i < MAX_STACKSIZE; i++)
        mStack[i] = STACK_ERROR;
}


StackEmulator::~StackEmulator(void)
{
}

/*
 0x01234: mov [esp+C], eax
 --> modifyFrom(0xC,0x01234)
*/
void StackEmulator::modifyFrom(int relative_offset, duint addr)
{
    const unsigned int internal_pointer = pointerByOffset(-1 * relative_offset);
    mStack[internal_pointer] = addr;
}
/*
 0x01234: pop eax
 --> popFrom(0x01234), because it is:

 mov eax, [esp]
 add esp, -1    ; one to the past

*/
void StackEmulator::popFrom(duint addr)
{
    moveStackpointerBack(+1);
}
/*
 0x01234: push eax
 --> pushFrom(0x01234), because it is:

 add esp, +1
 mov [esp], eax  ; one to the future
*/
void StackEmulator::pushFrom(duint addr)
{
    moveStackpointerBack(-1);
    mStack[mStackpointer] = addr;
}
/*
 0x01234: add esp, 0xA
 --> moveStackpointer(0xA)

 offset = offset to the past
*/
void StackEmulator::moveStackpointerBack(int offset)
{
    mStackpointer = pointerByOffset(-offset);
}

unsigned int StackEmulator::pointerByOffset(int offset) const
{
    return (mStackpointer + ((offset + MAX_STACKSIZE) % MAX_STACKSIZE) + MAX_STACKSIZE) % MAX_STACKSIZE;
}

/* returns addr from last access

   0x155: mov [esp+8], eax

   lastAccessAt(0x8)  would be 0x155

*/
duint StackEmulator::lastAccessAtOffset(int offset) const
{
    int p = pointerByOffset(-offset);
    return mStack[p];
}



void StackEmulator::emulate(const DISASM* BeaStruct)
{
    /* track all events:
    - sub/add esp
    - mov [esp+x], ???
    - push/pop
    */
    //

    const duint addr = BeaStruct->VirtualAddr;                    // --> 00401301

    if(_isPush(BeaStruct))
    {
        // "0x123  push eax" --> remember 0x123
        pushFrom(addr);
    }
    else if(_isPop(BeaStruct))
    {
        // "0x125  pop ebp" --> remember 0x125
        popFrom(addr);
    }
    else if(_isSub(BeaStruct))
    {

        if((strcmp(BeaStruct->Argument1.ArgMnemonic, "esp ") == 0))
        {
            // "sub esp, ???"
            moveStackpointerBack(BeaStruct->Instruction.Immediat / REGISTER_SIZE);
        }

    }
    else if(_isAdd(BeaStruct))
    {

        if((strcmp(BeaStruct->Argument1.ArgMnemonic, "esp ") == 0))
        {
            // "add esp, ???"
            moveStackpointerBack(BeaStruct->Instruction.Immediat / -REGISTER_SIZE);
        }

    }
    else
    {
        // "00401301: mov dword ptr ss:[esp+04h], 0040400Eh"
        if((BeaStruct->Argument1.AccessMode == WRITE)
                && (BeaStruct->Argument1.ArgType & MEMORY_TYPE)
                && (BeaStruct->Argument1.Memory.BaseRegister & REG4)
                && (BeaStruct->Argument1.SegmentReg & SSReg)
          )
        {
            Int64 offset = BeaStruct->Argument1.Memory.Displacement;  // --> 04h
            duint addr = BeaStruct->VirtualAddr;                    // --> 00401301

            modifyFrom(offset / REGISTER_SIZE, addr);



        }
    }



}

};