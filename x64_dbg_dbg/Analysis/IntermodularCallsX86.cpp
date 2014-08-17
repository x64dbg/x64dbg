#ifndef _WIN64


#include "IntermodularCalls.h"
#include "AnalysisRunner.h"
#include "../console.h"
namespace tr4ce
{

#define _isCall(disasm)  ((disasm.Instruction.Opcode == 0xE8) && (disasm.Instruction.BranchType) && (disasm.Instruction.BranchType!=RetType) && !(disasm.Argument1.ArgType &REGISTER_TYPE))


IntermodularCalls::IntermodularCalls(AnalysisRunner* parent) : ICommand(parent)
{

}

void IntermodularCalls::clear()
{
    numberOfApiCalls = 0;
    numberOfCalls = 0;
}

void IntermodularCalls::see(const  Instruction_t* currentInstruction, const StackEmulator* stackState, const RegisterEmulator* regState)
{

    if((currentInstruction->BeaStruct.Instruction.Opcode != 0xFF) && (_isCall(currentInstruction->BeaStruct)))
    {
        // current instructions contains a call
        // extract from "call 0x123" --> instruction at 0x123
        Instruction_t callTarget;
        int len = mParent->instruction(currentInstruction->BeaStruct.Instruction.AddrValue, &callTarget);
        if(len != UNKNOWN_OPCODE)
        {
            // call target was correctly disassembled before
            if(callTarget.BeaStruct.Instruction.Opcode == 0xFF)
            {

                // the opcode 0xFF "jmp" tells us that the current call is a call to a dll-function
                numberOfApiCalls++;
                numberOfCalls++;
                // does the TitanEngine provides us a label?
                char labelText[MAX_LABEL_SIZE];
                bool hasLabel = DbgGetLabelAt(callTarget.BeaStruct.Argument1.Memory.Displacement, SEG_DEFAULT, labelText);
                if(hasLabel)
                {

                    // we have a label from TitanEngine --> look up function header in database
                    FunctionInfo_t f = mParent->FunctionInformation()->find(labelText);
                    if(!f.invalid)
                    {
                        // yeah we know everything about the dll-call!
                        std::string functionComment;
                        functionComment = f.ReturnType + " " + f.Name + "(...)";
                        DbgSetAutoCommentAt(currentInstruction->BeaStruct.VirtualAddr, functionComment.c_str());


                        // set comments for the arguments
                        for(auto i = 0; i < f.Arguments.size(); i++)
                        {
                            std::string ArgComment = f.arg(i).Type + " " + f.arg(i).Name;
                            uint commentAddr = stackState->lastAccessAtOffset(f.Arguments.size() - i - 1);
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
                }
            }
            else
            {
                numberOfCalls++;
            }
        }
    }
}

bool IntermodularCalls::think()
{
    StackEmulator stack;

    dprintf("[StaticAnalysis:IntermodularCalls] found %i calls\n", numberOfCalls);
    dprintf("[StaticAnalysis:IntermodularCalls] of which are %i intermodular calls\n", numberOfApiCalls);

    return true;
}

void IntermodularCalls::unknownOpCode(const  DISASM* disasm)
{
    // current instruction wasn't correctly disassembled, so assuming worst-case

}

};

#endif // _WIN64