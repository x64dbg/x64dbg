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
#include "StackEmulator.h"
#include "FunctionInfo.h"
#include "../disasm_fast.h"

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
        if(n->outgoing != NULL)
        {
            if(n->outgoing->type == fa::CALL)
            {
                tDebug("api: found call at "fhex" to "fhex" \n", (duint)Instr.BeaStruct.VirtualAddr, n->outgoing->end->va);
                // test if the end has an edge, too
                tDebug("api: opcode is %x \n", (((n->outgoing->end))->instruction->BeaStruct.Instruction.Opcode) & 0xFF);
                tDebug("api: opcode is %x \n", Analysis->graph()->node(n->outgoing->end->va)->instruction->BeaStruct.Instruction.Opcode);

                const BASIC_INSTRUCTION_INFO* basicinfo = &n->outgoing->end->instruction->BasicInfo;
                duint ptr = basicinfo->addr > 0 ? basicinfo->addr : basicinfo->memory.value;
                char label[MAX_LABEL_SIZE] = "";
                bool found = DbgGetLabelAt(ptr, SEG_DEFAULT, label) && !labelget(ptr, label); //a non-user label


                return;
            }
        }

    }


}

};