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

duint CodeFollowPass::GetReferenceOperand(const ZydisDecodedInstruction & Context)
{
    for(int i = 0; i < Context.operandCount; i++)
    {
        auto operand = Context.operands[i];

        // Looking for immediate references
        if(operand.type == ZYDIS_OPERAND_TYPE_IMMEDIATE)
        {
            duint dest = (duint)operand.imm.value.u;

            if(ValidateAddress(dest))
                return dest;
        }
    }

    return 0;
}

duint CodeFollowPass::GetMemoryOperand(Zydis & Disasm, const ZydisDecodedInstruction & Context, bool* Indirect)
{
    if(Context.operandCount <= 0)
        return 0;

    // Only the first operand matters
    auto operand = Context.operands[0];

    // Jumps and calls only
    if(Disasm.IsCall() || Disasm.IsJump())
    {
        // Looking for memory references
        if(operand.type == ZYDIS_OPERAND_TYPE_MEMORY)
        {
            // Notify if the operand was indirect
            if(Indirect)
            {
                if(operand.mem.base != ZYDIS_REGISTER_NONE ||
                        operand.mem.index != ZYDIS_REGISTER_NONE ||
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

            duint dest = (duint)operand.mem.disp.value;

            if(ValidateAddress(dest))
                return dest;
        }
    }

    return 0;
}