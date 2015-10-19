#ifndef _CAPSTONE_GUI_H
#define _CAPSTONE_GUI_H

#include "capstone_wrapper.h"
#include "BeaTokenizer.h"
#include <QtCore>

class CapstoneTokenizer
{
public:
    enum class TokenType
    {
        //filling
        Comma = 0,
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
        XmmRegister,
        YmmRegister,
        ZmmRegister
    };

    struct TokenValue
    {
        int size; //value size (in bytes), zero means no value
        duint value; //actual value

        TokenValue(int size, duint value) :
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
        TokenType type; //token type
        QString text; //token text
        TokenValue value; //token value (if applicable)

        SingleToken(TokenType type, const QString & text, const TokenValue & value) :
            type(type),
            text(text),
            value(value)
        {
        }

        SingleToken(TokenType type, const QString & text) :
            SingleToken(type, text, TokenValue())
        {
        }
    };

    struct InstructionToken
    {
        std::vector<SingleToken> tokens; //list of tokens that form the instruction
        int x; //x of the first character

        InstructionToken()
        {
            tokens.clear();
            x = 0;
        }
    };

    CapstoneTokenizer(int maxModuleLength);
    bool Tokenize(duint addr, const unsigned char* data, int datasize, InstructionToken & instruction);
    BeaTokenizer::BeaSingleToken Convert(const SingleToken & cap);
    void UpdateConfig();
    void SetConfig(bool bUppercase, bool bTabbedMnemonic, bool bArgumentSpaces, bool bMemorySpaces);

private:
    Capstone _cp;
    InstructionToken _inst;
    int _maxModuleLength;
    bool _bUppercase;
    bool _bTabbedMnemonic;
    bool _bArgumentSpaces;
    bool _bMemorySpaces;

    void addToken(TokenType type, QString text, const TokenValue & value);
    void addToken(TokenType type, const QString & text);
    void addMemoryOperator(char operatorText);
    QString printValue(const TokenValue & value, bool expandModule, int maxModuleLength) const;

    bool tokenizePrefix();
    bool tokenizeMnemonic();
    bool tokenizeOperand(const cs_x86_op & op);
    bool tokenizeRegOperand(const cs_x86_op & op);
    bool tokenizeImmOperand(const cs_x86_op & op);
    bool tokenizeMemOperand(const cs_x86_op & op);
    bool tokenizeFpOperand(const cs_x86_op & op);
    bool tokenizeInvalidOperand(const cs_x86_op & op);
};

#endif //_CAPSTONE_GUI_H
