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
	ClientFunctionFinder::ClientFunctionFinder(AnalysisRunner  *an) : ClientInterface(an){

	}
	void ClientFunctionFinder::see(const Instruction_t Instr,const RegisterEmulator *reg,const StackEmulator *stack)
	{
		// test if node exists

		Node_t n;
		if(Analysis->graph()->find((duint)Instr.BeaStruct.VirtualAddr,&n)){
			dprintf("##found branch to   "fhex" \n",n.vaddr);

			// there is a branching!
// 			if(n.outEdge->type == fa::RET){
// 				// there is an api call
// 
// 				DbgSetAutoCommentAt((duint)Instr.BeaStruct.VirtualAddr, "ret");
// 				return;
// 			}
		}

		


	}

};