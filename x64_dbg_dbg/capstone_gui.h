#ifndef _CAPSTONE_GUI_H
#define _CAPSTONE_GUI_H

#include "_global.h"
#include "capstone_wrapper.h"

enum class SingleTokenType
{
    //filling
    Comma,
    Space,
    ArgumentSpace,
    MemoryOperatorSpace,
    //general instruction parts
    Prefix,
    Uncategorized,
    Address, //jump/call destinations or displacements inside memory
    Value,
    //mnemonics
    MnemonicNormal,
    MnemonicPushPop,
    MnemonicCall,
    MnemonicRet,
    MnemonicCondJump,
    MnemonicUncondJump,
    MnemonicNop,
    MnemonicFar,
    MnemonicInt3,
    //memory
    MemorySize,
    MemorySegment,
    MemoryBrackets,
    MemoryStackBrackets,
    MemoryBaseRegister,
    MemoryIndexRegister,
    MemoryScale,
    MemoryOperator, //'+', '-' and '*'
    //registers
    GeneralRegister,
    FpuRegister,
    MmxRegister,
    SseRegister
};

struct TokenValue
{
    int size; //value size (in bytes), zero means no value
    uint value; //actual value

    TokenValue(int size, uint value) :
        size(size),
        value(value)
    {
    }

    TokenValue() :
        size(0)
    {
    }
};

struct SingleToken
{
    SingleTokenType type; //token type
    std::string text; //token text
    TokenValue value; //token value (if applicable)

    SingleToken(SingleTokenType type, const std::string & text, const TokenValue & value) :
        type(type),
        text(text),
        value(value)
    {
    }

    SingleToken(SingleTokenType type, const std::string & text) :
        SingleToken(type, text, TokenValue())
    {
    }
};

struct InstructionToken
{
    std::vector<SingleToken> tokens; //list of tokens that form the instruction
    unsigned long hash; 
    int x; //x of the first character
};

class CapstoneTokenizer
{
public:
    bool Tokenize(uint addr, const unsigned char* data, int datasize, InstructionToken & instruction);

private:
    Capstone _cp;
    InstructionToken _inst;

    void addToken(SingleTokenType type, const std::string & text, const TokenValue & value);
    void addToken(SingleTokenType type, const std::string & text);
    bool tokenizePrefix();
    bool tokenizeMnemonic();
    bool tokenizeOperand(const cs_x86_op & op);
};

#endif //_CAPSTONE_GUI_H