#pragma once

#include "AnalysisPass.h"
#include "BasicBlock.h"
#include <zydis_wrapper.h>

class CodeFollowPass : public AnalysisPass
{
public:
    CodeFollowPass(duint VirtualStart, duint VirtualEnd, BBlockArray & MainBlocks);
    virtual ~CodeFollowPass();

    virtual const char* GetName() override;
    virtual bool Analyse() override;

private:
    duint GetReferenceOperand(const ZydisDisassembledInstruction & Context);
    duint GetMemoryOperand(Zydis & Disasm, const ZydisDisassembledInstruction & Context, bool* Indirect);
};