#pragma once
#include "Meta.h"
#include "../_global.h"

namespace fa
{
class RegisterEmulator
{
    UInt64 mRCX;
    UInt64 mRDX;
    UInt64 mR8;
    UInt64 mR9;
public:
    RegisterEmulator();
    ~RegisterEmulator();

    void emulate(const DISASM* BeaStruct);

    const UInt64 rcx() const;
    const UInt64 rdx() const;
    const UInt64 r8() const;
    const UInt64 r9() const;
};

};