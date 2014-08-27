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
#include <sstream>

namespace fa
{
ClientFunctionFinder::ClientFunctionFinder(AnalysisRunner*  an) : ClientInterface(an)
{

}
void ClientFunctionFinder::see(const Instruction_t Instr, const RegisterEmulator* reg, const StackEmulator* stack)
{
    // ?????????????????????????????????????????????????????????????????????????????????
    // TO ALL CONTRIBUTORS
    // why is does "Analysis->graph()->find((duint)Instr.BeaStruct.VirtualAddr,n)" not
    // return the correct address to the pointer?
    //
    // the function "Analysis->graph()->node((duint)Instr.BeaStruct.VirtualAddr)" does return the correct address
    /* can be checked by
    Node_t *n;
    Analysis->graph()->find((duint)Instr.BeaStruct.VirtualAddr,n)
    dprintf("node address #1:  "fhex" \n",n);
    n = Analysis->graph()->node((duint)Instr.BeaStruct.VirtualAddr);
    dprintf("node address #2:  "fhex" \n",n);
    dprintf("are these the same? NO !? Why?");
    */
    // ?????????????????????????????????????????????????????????????????????????????????

    Node_t* n;
    if(Analysis->graph()->find((duint)Instr.BeaStruct.VirtualAddr, n))
    {
        n = Analysis->graph()->node((duint)Instr.BeaStruct.VirtualAddr);

        // is there an edge going out?
        if(n->outEdge != NULL)
        {
            // there is a branching!
            if(n->outEdge->type == fa::RET)
            {
                if((duint) n->outEdge->end->instruction.BeaStruct.Instruction.AddrValue != Analysis->oep())
                {
                    // internal call
                    DbgSetAutoFunctionAt((duint)n->outEdge->end->instruction.BeaStruct.Instruction.AddrValue, (duint)Instr.BeaStruct.VirtualAddr);
                    tDebug("add function from "fhex" to "fhex"\n", (duint) n->outEdge->end->instruction.BeaStruct.Instruction.AddrValue, (duint)Instr.BeaStruct.VirtualAddr);
                }

            }
        }

    }




}

};