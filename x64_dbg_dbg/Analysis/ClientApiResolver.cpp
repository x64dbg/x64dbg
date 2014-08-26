#include "../_global.h"
#include "../console.h"
#include "Meta.h"
#include "ClientInterface.h"
#include "ClientApiResolver.h"
#include "Node_t.h"
#include "Edge_t.h"
#include "FlowGraph.h"
#include "AnalysisRunner.h"
#include "StackEmulator.h"
#include "FunctionInfo.h"

namespace fa
{
ClientApiResolver::ClientApiResolver(AnalysisRunner* analys): ClientInterface(analys)
{

}
void ClientApiResolver::see(const Instruction_t Instr, const RegisterEmulator* reg, const StackEmulator* stack)
{



	Node_t* n;
	if(Analysis->graph()->find((duint)Instr.BeaStruct.VirtualAddr, n))
	{
		n = Analysis->graph()->node((duint)Instr.BeaStruct.VirtualAddr);

		// is there an edge going out?
		if(n->outEdge != NULL)
		{
			if(n->outEdge->type == fa::CALL)
			{
				ttDebug("api: found call at "fhex" to "fhex" \n", (duint)Instr.BeaStruct.VirtualAddr,n->outEdge->end->vaddr);
				// test if the end has an edge, too 
				ttDebug("api: opcode is %x \n",(((n->outEdge->end))->instruction.BeaStruct.Instruction.Opcode)&0xFF);
				ttDebug("api: opcode is %x \n",Analysis->graph()->node(n->outEdge->end->vaddr)->instruction.BeaStruct.Instruction.Opcode);

				if(((n->outEdge->end))->instruction.BeaStruct.Instruction.Opcode == 0xFF){
						tDebug("api: --> is API CALL\n", (duint)n->outEdge->end->instruction.BeaStruct.VirtualAddr);
						// there is an api call
						//DbgSetAutoCommentAt((duint)Instr.BeaStruct.VirtualAddr, "hi");
					
				}
				return;
			}
		}

	}
    // test if node exists
    //      Node_t n;
    //      if(Analysis->graph()->find((duint)Instr.BeaStruct.VirtualAddr,n)){
    //          dprintf("##found branch to   "fhex" \n",n.vaddr);
    //          DbgSetAutoCommentAt(Instr.BeaStruct.VirtualAddr, "hi");
    //          return;
    //          // there is a branching!
    //          if(n.outEdge->type == fa::EXTERNJMP){
    //              // there is an api call
    //
    //              DbgSetAutoCommentAt(Instr.BeaStruct.VirtualAddr, "hi");
    //              return;
    //
    // #ifndef _WIN64
    //
    //              char labelText[MAX_LABEL_SIZE];
    //              bool hasLabel = DbgGetLabelAt(n.outEdge->end->instruction.BeaStruct.Argument1.Memory.Displacement, SEG_DEFAULT, labelText);
    // #else
    //              char labelText[MAX_LABEL_SIZE];
    //              bool hasLabel = DbgGetLabelAt(n.instruction.BeaStruct.Instruction.AddrValue, SEG_DEFAULT, labelText);
    //
    // #endif
    //              if(hasLabel){
    //                  fa::FunctionInfo_t f = Analysis->functioninfo()->find(labelText);
    //                  if(!f.invalid)
    //                  {
    //                      // yeah we know everything about the dll-call!
    //                      std::string functionComment;
    //                      functionComment = f.ReturnType + " " + f.Name + "(...)";
    //                      DbgSetAutoCommentAt(n.instruction.BeaStruct.VirtualAddr, functionComment.c_str());
    //
    // #ifndef _WIN64
    //                      // set comments for the arguments
    //                      for(size_t i = 0; i < f.Arguments.size(); i++)
    //                      {
    //                          std::string ArgComment = f.arg(i).Type + " " + f.arg(i).Name;
    //                          uint commentAddr = stack->lastAccessAtOffset(f.Arguments.size() - i - 1);
    //                          if(commentAddr != STACK_ERROR)
    //                          {
    //                              DbgSetAutoCommentAt(commentAddr, ArgComment.c_str());
    //                          }
    //                          else
    //                          {
    //                              // we have more arguments in the function descriptions than parameters on the stack
    //                              break;
    //                          }
    //                      }
    // #else
    //                      std::string functionComment;
    //                      functionComment = f.ReturnType + " " + f.Name + "(...)";
    //                      DbgSetAutoCommentAt(n.instruction.BeaStruct.VirtualAddr, functionComment.c_str());
    //
    //                      if(f.Arguments.size() > 0)
    //                      {
    //                          std::string ArgComment = f.arg(0).Type + " " + f.arg(0).Name;
    //                          DbgSetAutoCommentAt(reg->rcx(), ArgComment.c_str());
    //                      }
    //                      if(f.Arguments.size() > 1)
    //                      {
    //                          std::string ArgComment = f.arg(1).Type + " " + f.arg(1).Name;
    //                          DbgSetCommentAt(reg->rdx(), ArgComment.c_str());
    //                      }
    //                      if(f.Arguments.size() > 2)
    //                      {
    //                          std::string ArgComment = f.arg(2).Type + " " + f.arg(2).Name;
    //                          DbgSetAutoCommentAt(reg->r8(), ArgComment.c_str());
    //                      }
    //                      if(f.Arguments.size() > 3)
    //                      {
    //                          std::string ArgComment = f.arg(3).Type + " " + f.arg(3).Name;
    //                          DbgSetAutoCommentAt(reg->r9(), ArgComment.c_str());
    //                      }
    //                      if(f.Arguments.size() > 4)
    //                      {
    //                          // set comments for the arguments
    //                          for(auto i = 4; i < f.Arguments.size(); i++)
    //                          {
    //                              std::string ArgComment = f.arg(i).Type + " " + f.arg(i).Name;
    //                              uint commentAddr = stack->lastAccessAtOffset(f.Arguments.size() - i - 1);
    //                              if(commentAddr != STACK_ERROR)
    //                              {
    //                                  DbgSetAutoCommentAt(commentAddr, ArgComment.c_str());
    //                              }
    //                              else
    //                              {
    //                                  // we have more arguments in the function descriptions than parameters on the stack
    //                                  break;
    //                              }
    //                          }
    //                      }
    // #endif
    //
    //                  }
    //
    //              }
    //          }
    //      }


}

};