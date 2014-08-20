#include "../_global.h"
#include "../console.h"
#include "Meta.h"
#include "ClientInterface.h"
#include "ClientFunctionFinder.h"
#include "Node_t.h"
#include "Edge_t.h"
#include "FlowGraph.h"
#include "AnalysisRunner.h"
#include "StackEmulator.h"

namespace fa
{

	void ClientFunctionFinder::see(const Instruction_t Instr,const RegisterEmulator *reg,const StackEmulator *stack)
	{
		// test if node exists
		Node_t n;
		if(Analysis->graph()->find(Instr.BeaStruct.VirtualAddr,&n)){
			// there is a branching!
			if(n.outEdge->type == fa::RET){
				// just use the edge from "RET" back to the start of the function
				UInt64 startAddr = n.outEdge->end->instruction.BeaStruct.VirtualAddr;
				UInt64 endAddr = Instr.BeaStruct.VirtualAddr;
				DbgSetAutoFunctionAt(startAddr,endAddr);
				// easy, isn't it?
			}
		}


	}

};