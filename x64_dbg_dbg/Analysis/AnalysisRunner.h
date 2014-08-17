#pragma once
#include "../_global.h"
#include <map>
#include "Meta.h"
#include "StackEmulator.h"
#include "RegisterEmulator.h"
#include "ApiDB.h"

namespace tr4ce
{

class IntermodularCalls;
class CallDetector;


class AnalysisRunner
{
    std::map<UInt64, Instruction_t> mInstructionsBuffer;
    duint mBaseAddress;
    duint mSize;

    IntermodularCalls* _Calls;
	CallDetector* _Func;
    ApiDB* mApiDb;


    unsigned char* mCodeMemory;
    UIntPtr currentEIP;
    UInt64 currentVirtualAddr;

protected:
    // forwarding
    void see(const Instruction_t* disasm, const  StackEmulator* stack, const RegisterEmulator* regState);
    void clear();
    void think();
    void initialise();
    void unknownOpCode(const DISASM* disasm);

private:
    void run();
    void publishInstructions();

public:

    ApiDB* FunctionInformation() const;
    void setFunctionInformation(ApiDB* api);

    AnalysisRunner(duint BaseAddress, duint Size);
    ~AnalysisRunner(void);

    void start();



    int instruction(UInt64 va, Instruction_t* instr) const;
	std::map<UInt64, Instruction_t>::const_iterator instructionIter(UInt64 va) const;
	std::map<UInt64, Instruction_t>::const_iterator lastInstruction() const;

	duint base() const;


};

};