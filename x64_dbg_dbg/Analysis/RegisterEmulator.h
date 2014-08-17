#pragma once
#include "meta.h"
#include "../_global.h"
namespace tr4ce
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

    const uint rcx() const;
    const uint rdx() const;
    const uint r8() const;
    const uint r9() const;
};

};