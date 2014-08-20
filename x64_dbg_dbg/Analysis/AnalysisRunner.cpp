#include "AnalysisRunner.h"
#include "../_global.h"
#include "../console.h"
#include "Meta.h"

#include "Node_t.h"
#include "Edge_t.h"
#include "StackEmulator.h"
#include "RegisterEmulator.h"
#include "FunctionInfo.h"
#include "FlowGraph.h"

/* the idea is to start from the OEP and follow all instructions like an emulator would do it
 * and register all branching, i.e., EIP changes != eip++
 */

namespace fa
{

	AnalysisRunner::AnalysisRunner(const duint addrOEP,const duint BaseAddress,const duint Size) : OEP(addrOEP), baseAddress(BaseAddress), codeSize(Size)
	{
		// we start at the original entry point
		disasmRoot.insert(addrOEP);
		codeWasCopied = initialise();
		if(codeWasCopied){
			Grph = new FlowGraph;
			Node_t *cipNode = new Node_t(OEP);
			Grph->insertNode(cipNode);
		}
		
	}

	bool AnalysisRunner::initialise()
	{
		// copy the code section to play with it
		codeBuffer = new unsigned char[codeSize];
		if(!DbgMemRead(baseAddress, codeBuffer, codeSize))
		{
			//ERROR: copying did not work
			dputs("[StaticAnalysis] could not read memory ...");
			return false;
		}
		return true;

	}


	AnalysisRunner::~AnalysisRunner(void)
	{
		if(codeWasCopied)
			delete Grph;
		codeWasCopied = false;
		
	}

	void AnalysisRunner::start()
	{
		if(!codeWasCopied)
			return;
		dputs("[StaticAnalysis] analysis started ...");
		buildGraph();
		emulateInstructions();
		dputs("[StaticAnalysis] analysis finished ...");
	}


	bool AnalysisRunner::disasmChilds(duint rootAddress)
	{
		// this function will run until an unconditional branching (JMP,RET) or unkown OpCode
		if (contains(instructionBuffer,(UInt64)rootAddress))
		{
			// we already were here -->stop!
			return true;
		}

		DISASM disasm;
		memset(&disasm, 0, sizeof(disasm));
#ifdef _WIN64
		disasm.Archi = 64;
#endif 
		// indent pointer relative to current virtual address
		disasm.EIP = (UIntPtr)codeBuffer  + (rootAddress - baseAddress); 
		disasm.VirtualAddr = (UInt64)rootAddress;

		// while there is code in the buffer
		while(disasm.VirtualAddr - baseAddress < codeSize)
		{
			// disassemble instruction
			int instrLength = Disasm(&disasm);
			// everything ok?
			if(instrLength != UNKNOWN_OPCODE)
			{
				// create a new structure
				Instruction_t instr(&disasm, instrLength);
				// cache instruction
				instructionBuffer.insert(std::pair<UInt64, Instruction_t>(disasm.VirtualAddr, instr));

				// handle all kind of branching (cond. jumps, uncond. jumps, ret, unkown OpCode, calls)
				if(disasm.Instruction.BranchType){
					// there is a branch
					Node_t *startNode = new Node_t(instr); 
					Node_t *endNode;

					const Int32 BT = disasm.Instruction.BranchType;

					if(BT == RetType){
						// end of a function 
						// --> start was probably "rootAddress"
						// --> edge from current VA to rootAddress
						endNode = new Node_t(instruction_t(rootAddress));
						Edge_t *e = new Edge_t(startNode,endNode,fa::RET);
						Grph->insertEdge(e);
						// no need to disassemble more
						// "rootAddress" is from "call <rootAddress>" 
						return true;
					}else{
						// this is a "call","jmp","ret","jne","jnz","jz",...
						// were we are going to?
						endNode = new Node_t(instruction_t(rootAddress));
						// determine the type of flow-control-modification
						fa::EdgeType currentEdgeType;
						
						if(BT == CallType){
							// simply a call
							currentEdgeType = fa::CALL;
						}else if(BT == JmpType){
							// external Jump ?
							// TODO: this is currently x86 only!
							bool extjmp;
#ifndef _WIN64
							extjmp =( disasm.Instruction.Opcode == 0xFF);
#else
							char labelText[MAX_LABEL_SIZE];
							bool hasLabel = DbgGetLabelAt(disasm.Instruction.AddrValue, SEG_DEFAULT, labelText);
							if(hasLabel)
							{
								// we have a label --> look up function header in database
								FunctionInfo_t f = ApiInfo->find(labelText);
								extjmp = !f.invalid;
							}
#endif
							if(extjmp){
								currentEdgeType = fa::EXTERNJMP;
							}else{
								currentEdgeType = fa::UNCONDJMP;
							}
						}else{
							// all other branches are conditional jumps
							currentEdgeType = fa::CONDJMP;
						}
						// create a new edge for this EIP change
						Edge_t *edge = new Edge_t(startNode,endNode,currentEdgeType);
						Grph->insertEdge(edge);

						if(currentEdgeType != fa::EXTERNJMP){
							// the target must be disassembled too --> insert on todo-list
							disasmRoot.insert(endNode->virtualAddress);
						}

						if( BT == JmpType ){
							// unconditional flow change --> do not disassemble the next instruction
							return true;
						}

					}
				}
			}
			else
			{
				// unknown OpCode
				// --> don't know how to handle
				// --> pray everything will done correctly
				return false;
			}
			// we are allowed to analyze the next instruction
			disasm.EIP += instrLength;
			disasm.VirtualAddr += instrLength;
		}

		return true;
	}

	void AnalysisRunner::buildGraph()
	{
		// execute todo list
		while(disasmRoot.size() != 0){
			// get any address
			UInt64 addr = *(disasmRoot.begin());
			// did we already analyzed that address?
			if(!contains(instructionBuffer,addr)){
				// analyze until branching
				disasmChilds(addr);
			}
			// delete it from todo list
			disasmRoot.erase(disasmRoot.find(addr));
		}
		// we do not need the buffer anymore
		delete codeBuffer;
	}

	std::map<UInt64, Instruction_t>::const_iterator AnalysisRunner::instruction(UInt64 addr) const
	{
		return instructionBuffer.find(addr);
	}
	std::map<UInt64, Instruction_t>::const_iterator AnalysisRunner::lastInstruction() const
	{
		return instructionBuffer.end();
	}
	duint AnalysisRunner::base() const
	{
		return baseAddress;
	}

	void AnalysisRunner::emulateInstructions()
	{
		// track important events
		Stack = new StackEmulator;
		Register = new RegisterEmulator;
		functionInfo = new FunctionInfo;

		// run through instructions in a linear way - each instruction once
		std::map<UInt64, Instruction_t>::iterator it = instructionBuffer.begin();
		while(it!=instructionBuffer.end()){
			// save important values
			Stack->emulate(&(it->second.BeaStruct));
			Register->emulate(&(it->second.BeaStruct));

			// next instruction
			it++;
		}

		delete Stack;
		delete Register;
		delete functionInfo;
	}

	UInt64 AnalysisRunner::oep() const
	{
		return OEP;
	}

	duint AnalysisRunner::size() const
	{
		return codeSize;
	}

	FlowGraph* AnalysisRunner::graph() const
	{
		return Grph;
	}

	fa::Instruction_t AnalysisRunner::instruction_t( UInt64 va ) const
	{
		return instruction(va)->second;
	}

	FunctionInfo* AnalysisRunner::functioninfo() 
	{
		return functionInfo;
	}

};