



#include "IntermodularCalls.h"
#include "AnalysisRunner.h"
#include "../console.h"
#include "FunctionDetector.h"


namespace tr4ce
{

	
#ifndef _WIN64
#define _isCall(disasm)  ((disasm.Instruction.Opcode == 0xE8) && (disasm.Instruction.BranchType) && (disasm.Instruction.BranchType!=RetType) && !(disasm.Argument1.ArgType &REGISTER_TYPE))
#else
#define _isCall(disasm)  ((disasm.Instruction.BranchType==CallType) && (disasm.Instruction.BranchType!=RetType) && !(disasm.Argument1.ArgType &REGISTER_TYPE))
#endif


	CallDetector::CallDetector(AnalysisRunner* parent) : ICommand(parent)
	{

	}

	void CallDetector::clear()
	{
		numberOfFunctions = 0;
	}
	#ifndef _WIN64
	void CallDetector::see(const  Instruction_t* currentInstruction, const StackEmulator* stackState, const RegisterEmulator* regState)
	{

		if((currentInstruction->BeaStruct.Instruction.Opcode != 0xFF) && (_isCall(currentInstruction->BeaStruct)))
		{
			Instruction_t callTarget;
			int len = mParent->instruction(currentInstruction->BeaStruct.Instruction.AddrValue, &callTarget);
			if(len != UNKNOWN_OPCODE)
			{
				if((callTarget.BeaStruct.Instruction.Opcode != 0xFF) && (currentInstruction->BeaStruct.Instruction.AddrValue != mParent->base())){
					dprintf("[StaticAnalysis:CallDetector] call at %x \n",currentInstruction->BeaStruct.VirtualAddr);
					dprintf("[StaticAnalysis:CallDetector] call to %x \n",currentInstruction->BeaStruct.Instruction.AddrValue);
					Call_t c(0);
					c.startAddress = currentInstruction->BeaStruct.Instruction.AddrValue;
					mCalls.insert(c);
				}
			}
		}
	}
#else
	void CallDetector::see(const  Instruction_t* currentInstruction, const StackEmulator* stackState, const RegisterEmulator* regState)
	{

// 		if((_isCall(currentInstruction->BeaStruct)))
// 		{
// 			char labelText[MAX_LABEL_SIZE];
// 			bool hasLabel = DbgGetLabelAt(currentInstruction->BeaStruct.Instruction.AddrValue, SEG_DEFAULT, labelText);
// 			if(hasLabel)
// 			{
// 				// we have NO label from TitanEngine --> custom call
// 				FunctionInfo_t f = mParent->FunctionInformation()->find(labelText);
// 				if(f.invalid)
// 				{
// 					numberOfFunctions++;
// 					dprintf("[StaticAnalysis:CallDetector] call at %x \n",currentInstruction->BeaStruct.VirtualAddr);
// 					dprintf("[StaticAnalysis:CallDetector] call to %x \n",currentInstruction->BeaStruct.Instruction.AddrValue);
// 					Call_t c(0);
// 					c.startAddress = currentInstruction->BeaStruct.Instruction.AddrValue;
// 					mCalls.insert(c);
// 
// 				}
// 			}
// 
// 	
// 		}
	}
	#endif // _WIN64
	bool CallDetector::think()
	{
#ifndef _WIN64
		const int RETOPCODE = 0xC2;
#else
		const int RETOPCODE = 0xC3;
#endif

		for each(Call_t c in mCalls){
			dprintf("[StaticAnalysis:CallDetector] think about %x \n",c.startAddress);
			Instruction_t t;

			std::map<UInt64, Instruction_t>::const_iterator code = mParent->instructionIter(c.startAddress);

			while(code->second.BeaStruct.Instruction.Opcode != RETOPCODE){
				code++;
			}
			
			DbgSetAutoFunctionAt(c.startAddress,code->second.BeaStruct.VirtualAddr);
		}

		dprintf("[StaticAnalysis:CallDetector] found %i functions\n", numberOfFunctions);

		return true;
	}

	void CallDetector::unknownOpCode(const  DISASM* disasm)
	{
		// current instruction wasn't correctly disassembled, so assuming worst-case

	}

	
	};


