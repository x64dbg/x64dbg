#include "capstone_gui.h"

bool CapstoneTokenizer::Tokenize(uint addr, const unsigned char* data, int datasize, InstructionToken & instruction)
{
    if (!_cp.Disassemble(addr, data, datasize))
        return false;

    if (!tokenizeMnemonic())
        return false;

    for (int i = 0; i < _cp.OpCount(); i++)
    {
        if (!tokenizeOperand(_cp[i]))
            return false;
    }

    instruction = _inst;

    return true;
}

void CapstoneTokenizer::addToken(SingleTokenType type, const std::string & text, const TokenValue & value)
{
    //TODO: checks
    _inst.tokens.push_back(SingleToken(type, text, value));
}

void CapstoneTokenizer::addToken(SingleTokenType type, const std::string & text)
{
    addToken(type, text, TokenValue());
}

bool CapstoneTokenizer::tokenizePrefix()
{
    return true;
}

bool CapstoneTokenizer::tokenizeMnemonic()
{

    return true;
}

bool CapstoneTokenizer::tokenizeOperand(const cs_x86_op & op)
{
    switch (op.type)
    {
    case X86_OP_INVALID:
        break;
    case X86_OP_REG:
        break;
    case X86_OP_IMM:
        break;
    case X86_OP_MEM:
        break;
    case X86_OP_FP:
        break;
    default:
        return false;
    }
    return true;
}