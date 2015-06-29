#include "AnalysisPass.h"
#include "CodeFollowPass.h"

CodeFollowPass::CodeFollowPass(uint VirtualStart, uint VirtualEnd)
    : AnalysisPass(VirtualStart, VirtualEnd)
{

}

CodeFollowPass::~CodeFollowPass()
{
}

bool CodeFollowPass::Analyse()
{
    // First gather all possible function references with a linear scan-down
    //
    // This includes:
    // call <addr>
    // jmp <addr>
    // push <addr>
    // mov XXX, <addr>
    return false;
}

uint CodeFollowPass::GetReferenceOperand(const cs_x86 & Context)
{
    for(int i = 0; i < Context.op_count; i++)
    {
        auto operand = Context.operands[i];

        // Looking for immediate references
        if(operand.type == X86_OP_IMM)
        {
            uint dest = (uint)operand.imm;

            if(ValidateAddress(dest))
                return dest;
        }
    }

    return 0;
}

uint CodeFollowPass::GetMemoryOperand(Capstone & Disasm, const cs_x86 & Context, bool* Indirect)
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

            uint dest = (uint)operand.mem.disp;

            if(ValidateAddress(dest))
                return dest;
        }
    }

    return 0;
}