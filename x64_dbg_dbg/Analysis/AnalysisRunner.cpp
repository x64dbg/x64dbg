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
		disasmRoot.insert(std::make_pair<UInt64,UInt64>((duint)addrOEP,(duint)addrOEP));
		codeWasCopied = initialise();
		if(codeWasCopied){
			Grph = new FlowGraph;
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


	bool AnalysisRunner::disasmChilds(const UInt64 currentAddress,UInt64 parentAddress)
	{
		parentAddress = currentAddress;
		// Dear contributors (unless you are Chuck Norris):
		// 
		// if you think about 'optimizing' or 'debugging' this methods and then 
		// realize what a terrible mistake that was, please pray that this function will 
		// not fail and increment the following counter as a warning to the next guy:
		// 
		// total_hours_wasted_here = 3
		dprintf("-----------------------------------------------------------------------------\n");
		dprintf("processing subtree starting in node at address "fhex" as child of "fhex"\n", currentAddress,parentAddress);
		// this function will run until an unconditional branching (JMP,RET) or unkown OpCode
		if (contains(instructionBuffer,(UInt64)currentAddress))
		{
			// we already were here -->stop!
			return true;
		}
		
		// is there code?
		if((base() > currentAddress) && (base() + size() <= currentAddress)){
			return true;
		}

		DISASM disasm;
		memset(&disasm, 0, sizeof(disasm));
#ifdef _WIN64
		disasm.Archi = 64;
#endif 
		// indent pointer relative to current virtual address
		disasm.EIP = (UIntPtr)codeBuffer  + (currentAddress - baseAddress); 
		disasm.VirtualAddr = (UInt64)currentAddress;


		dprintf("loop end test: \n");
		dprintf(" va   "fhex"\n",disasm.VirtualAddr);
		dprintf(" base "fhex"\n",baseAddress);
		dprintf(" diff "fhex"\n",(UInt64)(disasm.VirtualAddr- baseAddress));
		// while there is code in the buffer
		while(disasm.VirtualAddr - baseAddress < codeSize)
		{
			// disassemble instruction
			const int instrLength = Disasm(&disasm);
			// everything ok?
			if(instrLength != UNKNOWN_OPCODE)
			{
				// create a new structure
				const Instruction_t instr(&disasm, instrLength);
				//dprintf("analyse %s at "fhex" \n", disasm.CompleteInstr,currentAddress);
				// cache instruction, we will later look at it
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
						endNode = new Node_t(parentAddress);
						Edge_t *e = new Edge_t(startNode,endNode,fa::RET);
						//dprintf("try to insert edge from "fhex" to "fhex" \n", e->start->vaddr, e->end->vaddr);
						dprintf("--> try to insert ret-edge from "fhex" to "fhex" \n", disasm.VirtualAddr, (UInt64) parentAddress);
						//###Grph->insertEdge(e);
						// no need to disassemble more
						// "rootAddress" is *only sometimes* from "call <rootAddress>" 
						return true;
					}else{
						// this is a "call","jmp","jne","jnz","jz",...
						// were we are going to?
						//### endNode = new Node_t(instruction_t(disasm.Instruction.AddrValue));
						// determine the type of flow-control-modification
						fa::EdgeType currentEdgeType;
						
						if(BT == CallType){
							// simply a call
							currentEdgeType = fa::CALL;
						}else if(BT == JmpType){
							// external Jump to known api call?
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
							if(extjmp ){
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
						dprintf("--> try to insert edge from "fhex" to "fhex" \n", disasm.VirtualAddr, (UInt64) disasm.Instruction.AddrValue);
						//####Grph->insertEdge(edge);

						if(currentEdgeType != fa::EXTERNJMP){
							// the target must be disassembled too --> insert on todo-list
							if(currentEdgeType ==  fa::CALL){
								// pass new address for correct resolving function-header addresses
								disasmRoot.insert(std::make_pair<UInt64,UInt64>(disasm.Instruction.AddrValue,disasm.VirtualAddr));
							}else{
								// we are in some functions --> propagate current function start
								disasmRoot.insert(std::make_pair<UInt64,UInt64>(disasm.Instruction.AddrValue,parentAddress));
							}
							dprintf("add todo at "fhex" from "fhex" \n", disasm.Instruction.AddrValue,disasm.VirtualAddr);
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

			dprintf("loop end test: \n");
			dprintf(" va   "fhex"\n",disasm.VirtualAddr);
			dprintf(" base "fhex"\n",baseAddress);
			dprintf(" diff "fhex"\n",(UInt64)(disasm.VirtualAddr- baseAddress));
		}

		return true;
	}

	void AnalysisRunner::buildGraph()
	{
		int i=15;
		// execute todo list
		while(disasmRoot.size() != 0){
			// get any address
			std::pair<UInt64,UInt64> addr = *(disasmRoot.begin());

			// does the address makes sense?
			if( (addr.first >= baseAddress) && (addr.first < baseAddress + codeSize) ){
				// did we already analyzed that address?
				if(!contains(instructionBuffer,addr.first)){
					// analyze until branching
					disasmChilds(addr.first,addr.second);
				}
			}
			// delete it from todo list
			disasmRoot.erase(disasmRoot.find(addr));
			
			if(i==0)
				return;
			i--;
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
	UInt64 AnalysisRunner::base() const
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