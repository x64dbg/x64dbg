#pragma once
#include "ICommand.h"
#include <list>
#include <set>
#include <map>

#include "meta.h"
#include "StackEmulator.h"
#include "RegisterEmulator.h"

namespace tr4ce
{



class IntermodularCalls : public ICommand
{
    unsigned int numberOfCalls;
    unsigned int numberOfApiCalls;

public:

    IntermodularCalls(AnalysisRunner* parent);

    void clear();
    void see(const Instruction_t* disasm, const StackEmulator* stack, const RegisterEmulator* regState);
    bool think();
    void unknownOpCode(const DISASM* disasm);
};

};