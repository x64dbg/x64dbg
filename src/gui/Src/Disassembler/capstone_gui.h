#ifndef _CAPSTONE_GUI_H
#define _CAPSTONE_GUI_H

#include <zydis_wrapper.h>
#include "RichTextPainter.h"
#include "Configuration.h"
#include <map>
#include <QHash>
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
        MnemonicUnusual,
        //values
        Address, //jump/call destinations or displacements inside memory
        Value,
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
        ZmmRegister,
        //last
        Last
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
            size(0),
            value(0)
        {
        }

        bool operator == (const TokenValue & rhs) const
        {
            return /*size == rhs.size &&*/ value == rhs.value;
        }
    };

    struct SingleToken
    {
        TokenType type; //token type
        QString text; //token text
        TokenValue value; //token value (if applicable)

        SingleToken() :
            type(TokenType::Uncategorized)
        {
        }

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

        bool operator == (const SingleToken & rhs) const
        {
            return type == rhs.type && text == rhs.text && value == rhs.value;
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

    struct TokenColor
    {
        RichTextPainter::CustomRichTextFlags flags;
        QColor color;
        QColor backgroundColor;

        TokenColor(QString color, QString backgroundColor)
        {
            if(color.length() && backgroundColor.length())
            {
                this->flags = RichTextPainter::FlagAll;
                this->color = ConfigColor(color);
                this->backgroundColor = ConfigColor(backgroundColor);
            }
            else if(color.length())
            {
                this->flags = RichTextPainter::FlagColor;
                this->color = ConfigColor(color);
            }
            else if(backgroundColor.length())
            {
                this->flags = RichTextPainter::FlagBackground;
                this->backgroundColor = ConfigColor(backgroundColor);
            }
            else
                this->flags = RichTextPainter::FlagNone;
        }

        TokenColor()
            : TokenColor("", "")
        {
        }
    };

    CapstoneTokenizer(int maxModuleLength);
    bool Tokenize(duint addr, const unsigned char* data, int datasize, InstructionToken & instruction);
    bool TokenizeData(const QString & datatype, const QString & data, InstructionToken & instruction);
    void UpdateConfig();
    void SetConfig(bool bUppercase, bool bTabbedMnemonic, bool bArgumentSpaces, bool bHidePointerSizes, bool bHideNormalSegments, bool bMemorySpaces, bool bNoHighlightOperands, bool bNoCurrentModuleText, bool b0xPrefixValues);
    int Size() const;
    const Zydis & GetCapstone() const;

    static void UpdateColors();
    static void UpdateStringPool();
    static void TokenToRichText(const InstructionToken & instr, RichTextPainter::List & richTextList, const SingleToken* highlightToken);
    static bool TokenFromX(const InstructionToken & instr, SingleToken & token, int x, CachedFontMetrics* fontMetrics);
    static bool IsHighlightableToken(const SingleToken & token);
    static bool TokenEquals(const SingleToken* a, const SingleToken* b, bool ignoreSize = true);
    static void addColorName(TokenType type, QString color, QString backgroundColor);
    static void addStringsToPool(const QString & regs);
    static bool tokenTextPoolEquals(const QString & a, const QString & b);

private:
    Zydis _cp;
    bool isNop;
    InstructionToken _inst;
    bool _success;
    int _maxModuleLength;
    bool _bUppercase;
    bool _bTabbedMnemonic;
    bool _bArgumentSpaces;
    bool _bHidePointerSizes;
    bool _bHideNormalSegments;
    bool _bMemorySpaces;
    bool _bNoHighlightOperands;
    bool _bNoCurrentModuleText;
    bool _b0xPrefixValues;
    TokenType _mnemonicType;

    void addToken(TokenType type, QString text, const TokenValue & value);
    void addToken(TokenType type, const QString & text);
    void addMemoryOperator(char operatorText);
    QString printValue(const TokenValue & value, bool expandModule, int maxModuleLength) const;

    static QHash<QString, int> stringPoolMap;
    static int poolId;

    bool tokenizePrefix();
    bool tokenizeMnemonic();
    bool tokenizeMnemonic(TokenType type, const QString & mnemonic);
    bool tokenizeOperand(const ZydisDecodedOperand & op);
    bool tokenizeRegOperand(const ZydisDecodedOperand & op);
    bool tokenizeImmOperand(const ZydisDecodedOperand & op);
    bool tokenizeMemOperand(const ZydisDecodedOperand & op);
    bool tokenizePtrOperand(const ZydisDecodedOperand & op);
    bool tokenizeInvalidOperand(const ZydisDecodedOperand & op);
};

#endif //_CAPSTONE_GUI_H
