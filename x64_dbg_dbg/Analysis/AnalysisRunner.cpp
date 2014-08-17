#include "AnalysisRunner.h"
#include "../_global.h"
#include "../console.h"
#include "meta.h"
#include "IntermodularCalls.h"
#include "FunctionDetector.h"
namespace tr4ce
{

AnalysisRunner::AnalysisRunner(duint BaseAddress, duint Size)
{
    // store all given information
    mBaseAddress = BaseAddress;
    mSize = Size;

	_Calls = new IntermodularCalls(this);
	_Func = new CallDetector(this);

    initialise();
    clear();
}


AnalysisRunner::~AnalysisRunner(void)
{
}

void AnalysisRunner::start()
{
    // do we have information about the function prototypes?
    if(!mApiDb->ok())
        return;
    dputs("[StaticAnalysis] analysis started ...");
    // remove all temp information
    clear();
    run();
    // see every instructions once
    publishInstructions();
    // do some magic
    think();
    dputs("[StaticAnalysis] analysis finished ...");
}

void AnalysisRunner::publishInstructions()
{
    StackEmulator Stack;
    RegisterEmulator Register;
    // show every sub-plugin the current instruction
    for each(auto currentInstruction in mInstructionsBuffer)
    {
        see(&currentInstruction.second, &Stack, &Register);
        Stack.emulate(&currentInstruction.second.BeaStruct);
        Register.emulate(&currentInstruction.second.BeaStruct);
    }
}

void AnalysisRunner::run()
{
    // this function will be run once

    // copy the code section
    mCodeMemory = new unsigned char[mSize];
    if(!DbgMemRead(mBaseAddress, mCodeMemory, mSize))
    {
        //ERROR: copying did not work
        dputs("[StaticAnalysis] could not read memory ...");
        return;
    }

    //loop over all instructions
    DISASM disasm;

    duint baseaddr = mBaseAddress;
    duint size = mSize;

    memset(&disasm, 0, sizeof(disasm));

#ifdef _WIN64
    disasm.Archi = 64;
#endif // _WIN64

    currentEIP = (UIntPtr)mCodeMemory;
    disasm.EIP = currentEIP;
    currentVirtualAddr = (UInt64)baseaddr;
    disasm.VirtualAddr = currentVirtualAddr;
    duint i = 0;

    for(duint i = 0; i < size;)
    {
        // disassemble instruction
        int len = Disasm(&disasm);
        // everything ok?
        if(len != UNKNOWN_OPCODE)
        {
            Instruction_t instr(&disasm, len);
            mInstructionsBuffer.insert(std::pair<UInt64, Instruction_t>(disasm.VirtualAddr, instr));
        }
        else
        {
            // something went wrong --> notify every subplugin
            unknownOpCode(&disasm);
            len = 1;
        }
        // we do not know if the struct DISASM gets destroyed on unkown opcodes --> use variables
        currentEIP += len;
        currentVirtualAddr += len;

        disasm.EIP = currentEIP;
        disasm.VirtualAddr = currentVirtualAddr;
        // move memory pointer
        i += len;
    }


    delete mCodeMemory;
}
void AnalysisRunner::clear()
{
    mInstructionsBuffer.clear();
    // forward to all sub-plugin
    _Calls->clear();
	_Func->clear();
}
void AnalysisRunner::think()
{
    // forward to all sub-plugin
    _Calls->think();
	_Func->think();
}
void AnalysisRunner::see(const Instruction_t* disasm, const StackEmulator* stack, const RegisterEmulator* regState)
{
    // forward to all sub-plugin
    _Calls->see(disasm, stack, regState);
	_Func->see(disasm, stack, regState);
}

void AnalysisRunner::unknownOpCode(const DISASM* disasm)
{
    // forward to all sub-plugin
    _Calls->unknownOpCode(disasm);
	_Func->unknownOpCode(disasm);
}

void AnalysisRunner::initialise()
{
    // forward to all sub-plugin
    _Calls->initialise(mBaseAddress, mSize);
	_Func->initialise(mBaseAddress, mSize);
}



ApiDB* AnalysisRunner::FunctionInformation() const
{
    return mApiDb;
}


void AnalysisRunner::setFunctionInformation(ApiDB* api)
{
    mApiDb = api;
}

// return instruction of address - if possible
int AnalysisRunner::instruction(UInt64 va, Instruction_t* instr) const
{
	if(tr4ce::contains(mInstructionsBuffer, va))
	{
		*instr = mInstructionsBuffer.at(va);
		return instr->Length;
	}
	else
	{
		return UNKNOWN_OPCODE;
	}
}
std::map<UInt64, Instruction_t>::const_iterator AnalysisRunner::instructionIter(UInt64 va) const
{
	return mInstructionsBuffer.find(va);
}
std::map<UInt64, Instruction_t>::const_iterator AnalysisRunner::lastInstruction() const
{
	return mInstructionsBuffer.end();
}

duint AnalysisRunner::base() const
{
	return mBaseAddress;
}

};