#include "RegisterEmulator.h"

namespace fa
{
#define _SAME(a,b)  ((strcmp(a ,b) == 0) )

RegisterEmulator::RegisterEmulator()
{
}


RegisterEmulator::~RegisterEmulator()
{
}

void RegisterEmulator::emulate(const DISASM* BeaStruct)
{
    if((BeaStruct->Argument1.AccessMode == WRITE) && ((BeaStruct->Argument1.ArgType & GENERAL_REG)))
    {
        // Unfortunately there is a bug in BeaEngine, so we cannot use
        // "if (BeaStruct->Argument1.ArgType & REG?)"
        if(_SAME(BeaStruct->Argument1.ArgMnemonic, "cl") ||
                _SAME(BeaStruct->Argument1.ArgMnemonic, "ch") ||
                _SAME(BeaStruct->Argument1.ArgMnemonic, "cx") ||
                _SAME(BeaStruct->Argument1.ArgMnemonic, "ecx") ||
                _SAME(BeaStruct->Argument1.ArgMnemonic, "rcx")
          )
        {
            mRCX = BeaStruct->VirtualAddr;
        }
        else if(_SAME(BeaStruct->Argument1.ArgMnemonic, "dl") ||
                _SAME(BeaStruct->Argument1.ArgMnemonic, "dh") ||
                _SAME(BeaStruct->Argument1.ArgMnemonic, "dx") ||
                _SAME(BeaStruct->Argument1.ArgMnemonic, "edx") ||
                _SAME(BeaStruct->Argument1.ArgMnemonic, "rdx")
               )
        {
            mRDX = BeaStruct->VirtualAddr;
        }
        else if(_SAME(BeaStruct->Argument1.ArgMnemonic, "r8") ||
                _SAME(BeaStruct->Argument1.ArgMnemonic, "r8d") ||
                _SAME(BeaStruct->Argument1.ArgMnemonic, "r8w") ||
                _SAME(BeaStruct->Argument1.ArgMnemonic, "r8b")
               )
        {
            mR8 = BeaStruct->VirtualAddr;
        }
        else if(_SAME(BeaStruct->Argument1.ArgMnemonic, "r9") ||
                _SAME(BeaStruct->Argument1.ArgMnemonic, "r9d") ||
                _SAME(BeaStruct->Argument1.ArgMnemonic, "r9w") ||
                _SAME(BeaStruct->Argument1.ArgMnemonic, "r9b")
               )
        {
            mR9 = BeaStruct->VirtualAddr;
        }

    }
}

const duint RegisterEmulator::rcx() const
{
    return mRCX;
}

const duint RegisterEmulator::rdx() const
{
    return mRDX;
}

const duint RegisterEmulator::r8() const
{
    return mR8;
}

const duint RegisterEmulator::r9() const
{
    return mR9;
}


};