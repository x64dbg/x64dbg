#pragma once
#include "../_global.h"
#include "Meta.h"
#include <set>

namespace fa
{
class AnalysisRunner;
class StackEmulator;
class RegisterEmulator;
class ClientInterface
{
public:
    AnalysisRunner* Analysis;

    ClientInterface(AnalysisRunner* analys);
    ~ClientInterface();

    virtual void see(const Instruction_t Instr, const RegisterEmulator* reg, const StackEmulator* stack) = 0;


};

};