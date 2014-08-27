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
#include "ClientApiResolver.h"
#include "ClientFunctionFinder.h"

/* the idea is to start from the OEP and follow all instructions like an emulator would do it
* and register all branching, i.e., EIP changes != eip++
*/

namespace fa
{

	AnalysisRunner::AnalysisRunner(const duint addrOEP, const duint BaseAddress, const duint Size) : OEP(addrOEP), baseAddress(BaseAddress), codeSize(Size)
	{
		// we start at the original entry point
		unknownRegion R;
		R.startAddress = (duint)addrOEP;
		R.headerAddress = (duint)addrOEP;

		explorationSpace.insert(R);

		codeWasCopied = initialise();
		if(codeWasCopied)
		{
			Grph = new FlowGraph(this);
		}


		interfaces.push_back(new ClientFunctionFinder(this));
		interfaces.push_back(new ClientApiResolver(this));

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
		explore();
		Grph->fillNodes();
		emulateInstructions();
		dputs("[StaticAnalysis] analysis finished ...");
	}

	void AnalysisRunner::explore()
	{
		// execute todo list
		while(explorationSpace.size() != 0)
		{
			// get any address
			unknownRegion region = *(explorationSpace.begin());

			// does the region makes sense?
			if((region.startAddress >= baseAddress) && (region.startAddress < baseAddress + codeSize))
			{
				// did we already analyzed that address?
				if(!contains(instructionsCache, region.startAddress))
				{
					// analyze until branching
					explore(region);
				}
			}
			// delete it from todo list
			explorationSpace.erase(explorationSpace.find(region));

		}
		// we do not need the buffer anymore
		delete codeBuffer;
	}

	bool AnalysisRunner::explore(const unknownRegion region)
	{
		// Dear contributors (unless you are Chuck Norris):
		//
		// if you think about 'optimizing' or 'debugging' this methods and then
		// realize what a terrible mistake that was, please pray that this function will
		// not fail and increment the following counter as a warning to the next guy:
		//
		// total_hours_wasted_here = 4

		/*    tDebug("explore address "fhex" as child of "fhex"\n", region.startAddress, region.headerAddress);*/

		// this function will run until an unconditional branching (JMP,RET) or unkown OpCode
		if(contains(instructionsCache, region.startAddress))
		{
			// we already were here -->stop!
			return true;
		}

		// is there code?
		if((base() > region.startAddress) || (base() + size() <= region.startAddress))
		{
			return true;
		}

		DISASM disasm;
		memset(&disasm, 0, sizeof(disasm));
#ifdef _WIN64
		disasm.Archi = 64;
#endif
		// indent pointer relative to current virtual address
		disasm.EIP = (UIntPtr)codeBuffer  + (region.startAddress - baseAddress);
		disasm.VirtualAddr = region.startAddress;

		// while there is code in the buffer
		while(disasm.VirtualAddr - baseAddress < codeSize)
		{
			// disassemble instruction
			const int instrLength = Disasm(&disasm);
			DISASM *disasm2 = new DISASM;
			*disasm2 = disasm;
			// everything ok?
			if(instrLength != UNKNOWN_OPCODE)
			{
				// create a new structure
				const Instruction_t instr(disasm2, instrLength);
				// cache instruction, we will later look at it
				instructionsCache.insert(std::pair<duint, Instruction_t>(disasm.VirtualAddr, instr));

				// handle all kind of branching (cond. jumps, uncond. jumps, ret, unkown OpCode, calls)
				if(disasm.Instruction.BranchType)
				{
					const duint NodeStart = disasm.VirtualAddr;
					duint NodeEnd;

					const Int32 BT = disasm.Instruction.BranchType;

					if(BT == RetType)
					{
						// end of a function
						// --> start was probably "headerAddress"
						// --> edge from current VA to headerAddress
						NodeEnd = region.headerAddress;
						//Edge_t *e = new Edge_t(startNode,endNode,fa::RET);
						//dprintf("try to insert edge from "fhex" to "fhex" \n", e->start->vaddr, e->end->vaddr);

						Grph->insertEdge(NodeStart, NodeEnd, fa::RET);
						// no need to disassemble more
						// "rootAddress" is *only sometimes* from "call <rootAddress>"
						return true;
					}
					else
					{
						// this is a "call","jmp","jne","jnz","jz",...
						// were we are going to?
						NodeEnd = disasm.Instruction.AddrValue;
						// determine the type of flow-control-modification
						fa::EdgeType currentEdgeType;

						if(BT == CallType)
						{
							currentEdgeType = fa::CALL;
						}
						else if(BT == JmpType)
						{
							// external Jump to known api call?
							bool extjmp;
#ifndef _WIN64
							extjmp = (disasm.Instruction.Opcode == 0xFF);
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
							currentEdgeType = extjmp ? fa::EXTERNJMP : fa::UNCONDJMP;
						}
						else
						{
							// all other branches are conditional jumps
							currentEdgeType = fa::CONDJMP;
						}
						// create a new edge for this EIP change
						//Edge_t *edge = new Edge_t(startNode,endNode,currentEdgeType);
						tDebug("--> try to insert edge from "fhex" to "fhex" \n",  NodeStart, NodeEnd);
						Grph->insertEdge(NodeStart, NodeEnd, currentEdgeType);
						//Grph->insertEdge(edge);

						if(true)
						{
							unknownRegion newRegion;
							newRegion.startAddress = (duint)disasm.Instruction.AddrValue;
							// the target must be disassembled too --> insert on todo-list
							if(currentEdgeType ==  fa::CALL)
							{
								// address value of call is new header
								newRegion.headerAddress = (duint)disasm.Instruction.AddrValue;
							}
							else
							{
								// we are in some functions --> propagate current function start
								newRegion.headerAddress = region.headerAddress;
							}
							tDebug("add todo at "fhex" with header "fhex" \n", newRegion.startAddress, newRegion.headerAddress);
							explorationSpace.insert(newRegion);
						}

						if(BT == JmpType)
						{
							// unconditional flow change --> do not disassemble the next instruction
							return true;
						}

					}
				}else{
					/* no branching, but we can look for pushed address to overcome
					00401919 | 68 3E 19 40 00           | push application_2014.40193E            | ;###
					0040191E | 6A 00                    | push 0                                  | ;###
					00401920 | 6A 00                    | push 0                                  | ;###
					00401922 | E8 A5 09 00 00           | call <JMP.&CreateThread>                | ;###
					*/

#define _isPush(x)  ((strcmp((x).Instruction.Mnemonic ,"push ") == 0) )

					if(_isPush(disasm)){
						const duint possibleAddr = disasm.Instruction.Immediat;
						if( (possibleAddr>=baseAddress) && (possibleAddr< baseAddress + codeSize)){
							unknownRegion R;
							R.startAddress = possibleAddr;
							R.headerAddress = possibleAddr;
							explorationSpace.insert(R);
						}
					}
				}
			}
			else
			{
				// unknown OpCode
				tDebug("unknown opcode at "fhex"  \n",(duint)disasm.VirtualAddr);
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
		std::map<duint, Instruction_t>::iterator it = instructionsCache.begin();
		while(it != instructionsCache.end())
		{
			// save important values
			Stack->emulate(&(it->second.BeaStruct));
			Register->emulate(&(it->second.BeaStruct));

			for(std::vector<ClientInterface*>::iterator itt = interfaces.begin();itt!=interfaces.end();itt++){
				(*itt)->see(it->second, Register, Stack);
			}
			// next instruction
			it++;
		}

		delete Stack;
		delete Register;
		delete functionInfo;
	}

	duint AnalysisRunner::oep() const
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

	const fa::Instruction_t* AnalysisRunner::instruction(duint va) const
	{
		return &(instructionsCache.find(va)->second);
	}

	FunctionInfo* AnalysisRunner::functioninfo()
	{
		return functionInfo;
	}

};