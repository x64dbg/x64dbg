#pragma once

#include "AnalysisPass.h"
#include "BasicBlock.h"

class Int3CoagulatorPass : public AnalysisPass
{
public:
    Int3CoagulatorPass(uint VirtualStart, uint VirtualEnd);
    virtual ~Int3CoagulatorPass();

    virtual bool Analyse() override;

private:
};