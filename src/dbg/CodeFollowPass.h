#pragma once

#include "AnalysisPass.h"
#include "BasicBlock.h"
#include <capstone_wrapper.h>

class CodeFollowPass : public AnalysisPass
{
public:
    CodeFollowPass(duint VirtualStart, duint VirtualEnd, BBlockArray & MainBlocks);
    virtual ~CodeFollowPass();

    virtual const char* GetName() override;
    virtual bool Analyse() override;

private:
    duint GetReferenceOperand(const cs_x86 & Context);
    duint GetMemoryOperand(Capstone & Disasm, const cs_x86 & Context, bool* Indirect);
};