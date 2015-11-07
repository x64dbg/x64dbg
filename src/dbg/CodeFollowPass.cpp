#include "AnalysisPass.h"
#include "CodeFollowPass.h"

CodeFollowPass::CodeFollowPass(duint VirtualStart, duint VirtualEnd, BBlockArray & MainBlocks)
    : AnalysisPass(VirtualStart, VirtualEnd, MainBlocks)
{

}

CodeFollowPass::~CodeFollowPass()
{
}

const char* CodeFollowPass::GetName()
{
    return "Code Follower";
}

bool CodeFollowPass::Analyse()
{
    // First gather all possible function references with certain tables
    //
    // This includes:
    // Main entry
    // Module exports
    // Module import references (?)
    // Control flow guard
    // RUNTIME_FUNCTION
    // TLS callbacks
    // FLS callbacks
    return false;
}

duint CodeFollowPass::GetReferenceOperand(const cs_x86 & Context)
{
    for(int i = 0; i < Context.op_count; i++)
    {
        auto operand = Context.operands[i];

        // Looking for immediate references
        if(operand.type == X86_OP_IMM)
        {
            duint dest = (duint)operand.imm;

            if(ValidateAddress(dest))
                return dest;
        }
    }

    return 0;
}

duint CodeFollowPass::GetMemoryOperand(Capstone & Disasm, const cs_x86 & Context, bool* Indirect)
{
    if(Context.op_count <= 0)
        return 0;

    // Only the first operand matters
    auto operand = Context.operands[0];

    // Jumps and calls only
    if(Disasm.InGroup(CS_GRP_CALL) || Disasm.InGroup(CS_GRP_JUMP))
    {
        // Looking for memory references
        if(operand.type == X86_OP_MEM)
        {
            // Notify if the operand was indirect
            if(Indirect)
            {
                if(operand.mem.base != X86_REG_INVALID ||
                        operand.mem.index != X86_REG_INVALID ||
                        operand.mem.scale != 0)
                {
                    *Indirect = true;
                    return 0;
                }
            }

            // TODO: Read pointer
            // TODO: Find virtual tables (new pass?)
            // TODO: Translate RIP-Relative
            return 0;

            duint dest = (duint)operand.mem.disp;

            if(ValidateAddress(dest))
                return dest;
        }
    }

    return 0;
}