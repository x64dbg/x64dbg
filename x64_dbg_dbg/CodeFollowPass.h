#pragma once

#include "AnalysisPass.h"
#include "BasicBlock.h"
#include "capstone_wrapper.h"

class CodeFollowPass : public AnalysisPass
{
public:
    CodeFollowPass(uint VirtualStart, uint VirtualEnd);
    virtual ~CodeFollowPass();

    virtual bool Analyse() override;

private:
    uint GetReferenceOperand(const cs_x86 & Context);
    uint GetMemoryOperand(Capstone & Disasm, const cs_x86 & Context, bool* Indirect);
};