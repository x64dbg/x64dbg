#include "../_global.h"
#include "../console.h"
#include "../addrinfo.h"
#include "Meta.h"
#include "ClientInterface.h"
#include "ClientApiResolver.h"
#include "Node_t.h"
#include "Edge_t.h"
#include "FlowGraph.h"
#include "AnalysisRunner.h"
#include "RegisterEmulator.h"
#include "StackEmulator.h"
#include "FunctionDB.h"
#include "../disasm_fast.h"

namespace fa
{
ClientApiResolver::ClientApiResolver(AnalysisRunner* analys): ClientInterface(analys)
{

}
void ClientApiResolver::see(const Instruction_t Instr, const RegisterEmulator* reg, const StackEmulator* stack)
{
    // save key strokes
    const duint addr = (duint)Instr.BeaStruct.VirtualAddr;
    Node_t* n = 0;
    // try to find a node at the current virtual address
    if(Analysis->graph()->find(addr, n))
    {
        // TODO: unfortunately the current value of "n" is NOT the address of the current node
        // --> call another function to get it anyway
        n = Analysis->graph()->node(addr);

        // is there an edge going out?
        if(n->outgoing.size() > 0)
        {
            tDebug("found call at "fhex"\n", addr);
            Edge_t* first_out_edge = *(n->outgoing.begin());
            if(first_out_edge->type == fa::CALL)
            {
                const BASIC_INSTRUCTION_INFO* basicinfo = &first_out_edge->end->instruction->BasicInfo;
                duint ptr = basicinfo->addr > 0 ? basicinfo->addr : basicinfo->memory.value;
                char label[MAX_LABEL_SIZE] = "";
                bool found = DbgGetLabelAt(ptr, SEG_DEFAULT, label) && !labelget(ptr, label); //a non-user label
                tDebug("has label %s\n", label);
                if(found)
                {
#ifdef _WIN64
                    // we have a label from TitanEngine --> look up function header in database
                    FunctionInfo_t f = Analysis->functionDB()->find(label);
                    if(!f.invalid)
                    {
                        // yeah we know everything about the dll-call!
                        std::string functionComment;
                        functionComment = f.ReturnType + " " + f.Name + "(...)";
                        DbgSetCommentAt(Instr.BeaStruct.VirtualAddr, functionComment.c_str());

                        if(f.Arguments.size() > 0)
                        {
                            std::string ArgComment = f.arg(0).Type + " " + f.arg(0).Name;
                            DbgSetCommentAt(reg->rcx(), ArgComment.c_str());
                        }
                        if(f.Arguments.size() > 1)
                        {
                            std::string ArgComment = f.arg(1).Type + " " + f.arg(1).Name;
                            DbgSetCommentAt(reg->rdx(), ArgComment.c_str());
                        }
                        if(f.Arguments.size() > 2)
                        {
                            std::string ArgComment = f.arg(2).Type + " " + f.arg(2).Name;
                            DbgSetCommentAt(reg->r8(), ArgComment.c_str());
                        }
                        if(f.Arguments.size() > 3)
                        {
                            std::string ArgComment = f.arg(3).Type + " " + f.arg(3).Name;
                            DbgSetCommentAt(reg->r9(), ArgComment.c_str());
                        }
                        if(f.Arguments.size() > 4)
                        {
                            // set comments for the arguments
                            for(auto i = 4; i < f.Arguments.size(); i++)
                            {
                                std::string ArgComment = f.arg(i).Type + " " + f.arg(i).Name;
                                duint commentAddr = stack->lastAccessAtOffset(f.Arguments.size() - i - 1);
                                if(commentAddr != STACK_ERROR)
                                {
                                    DbgSetCommentAt(commentAddr, ArgComment.c_str());
                                }
                                else
                                {
                                    // we have more arguments in the function descriptions than parameters on the stack
                                    break;
                                }
                            }
                        }

                    }
#else
                    FunctionInfo_t f = Analysis->functionDB()->find(label);
                    if(!f.invalid)
                    {
                        // yeah we know everything about the dll-call!
                        std::string functionComment;
                        functionComment = f.ReturnType + " " + f.Name + "(...)";
                        DbgSetAutoCommentAt((duint)Instr.BeaStruct.VirtualAddr, functionComment.c_str());


                        // set comments for the arguments
                        for(size_t i = 0; i < f.Arguments.size(); i++)
                        {
                            std::string ArgComment = f.arg(i).Type + " " + f.arg(i).Name;
                            duint commentAddr = stack->lastAccessAtOffset(f.Arguments.size() - i - 1);
                            if(commentAddr != STACK_ERROR)
                            {
                                DbgSetAutoCommentAt(commentAddr, ArgComment.c_str());
                            }
                            else
                            {
                                // we have more arguments in the function descriptions than parameters on the stack
                                break;
                            }
                        }

                    }
#endif
                }


                return;
            }
        }

    }


}

};