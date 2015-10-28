#ifndef BEATOKENIZER_H
#define BEATOKENIZER_H

#include <QMap>
#include <QSet>
#include "RichTextPainter.h"
#include "NewTypes.h"

class BeaTokenizer : RichTextPainter
{
public:
    enum BeaTokenType
    {
        //filling
        TokenComma = 0,
        TokenSpace,
        TokenArgumentSpace,
        TokenMemoryOperatorSpace,
        //general instruction parts
        TokenPrefix,
        TokenUncategorized,
        TokenAddress, //jump/call destinations or displacements inside memory
        TokenValue,
        //mnemonics
        TokenMnemonicNormal,
        TokenMnemonicPushPop,
        TokenMnemonicCall,
        TokenMnemonicRet,
        TokenMnemonicCondJump,
        TokenMnemonicUncondJump,
        TokenMnemonicNop,
        TokenMnemonicFar,
        TokenMnemonicInt3,
        //memory
        TokenMemorySize,
        TokenMemorySegment,
        TokenMemoryBrackets,
        TokenMemoryStackBrackets,
        TokenMemoryBaseRegister,
        TokenMemoryIndexRegister,
        TokenMemoryScale,
        TokenMemoryOperator, //'+', '-' and '*'
        //registers
        TokenGeneralRegister,
        TokenFpuRegister,
        TokenMmxRegister,
        TokenSseRegister
    };

    struct BeaTokenValue
    {
        int size; //value size
        int_t value; //value
    };

    struct BeaSingleToken
    {
        BeaTokenType type; //token type
        QString text; //text to display
        BeaTokenValue value; //jump destination/displacement/immediate
    };

    struct BeaInstructionToken
    {
        QList<BeaSingleToken> tokens; //list of tokens that form the instruction
        unsigned long hash; //complete instruction token checksum
        int x; //x of the first character
    };

    struct BeaTokenColor
    {
        QString color;
        QString backgroundColor;
    };

    static void Init();
    static void TokenToRichText(const BeaInstructionToken* instr, QList<RichTextPainter::CustomRichText_t>* richTextList, const BeaSingleToken* highlightToken);
    static bool TokenFromX(const BeaInstructionToken* instr, BeaSingleToken* token, int x, int charwidth);
    static bool IsHighlightableToken(const BeaSingleToken* token);
    static bool TokenEquals(const BeaSingleToken* a, const BeaSingleToken* b, bool ignoreSize = true);

private:
    //variables
    static QMap<BeaTokenType, BeaTokenColor> colorNamesMap;
    static void AddColorName(BeaTokenType type, QString color, QString backgroundColor);
};

#endif // BEATOKENIZER_H
