#ifndef BEATOKENIZER_H
#define BEATOKENIZER_H

#include <QList>
#include <QRect>
#include <QString>
#include <QStringList>
#include <QMap>
#include "BeaEngine.h"
#include "NewTypes.h"
#include "RichTextPainter.h"

class BeaTokenizer : RichTextPainter
{
public:
    BeaTokenizer();

    enum BeaTokenType
    {
        //filling
        TokenComma,
        TokenRequiredSpace,
        TokenOptionalSpace,
        TokenMemorySpace,
        //general instruction parts
        TokenPrefix,
        TokenGeneral,
        TokenCodeDest, //jump/call destinations
        TokenImmediat,
        //mnemonics
        TokenMnemonicNormal,
        TokenMnemonicPushPop,
        TokenMnemonicCallRet,
        TokenMnemonicCondJump,
        TokenMnemonicUncondJump,
        TokenMnemonicNop,
        //memory
        TokenMemorySize,
        TokenMemoryText,
        TokenMemorySegment,
        TokenMemoryBrackets,
        TokenMemoryBaseRegister,
        TokenMemoryIndexRegister,
        TokenMemoryScale,
        TokenMemoryDisplacement,
        TokenMemoryOperator,
        //registers
        TokenGeneralRegister,
        TokenFpuRegister,
        TokenMmxRegister,
        TokenSseRegister,
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
    static unsigned long HashInstruction(const DISASM* disasm);
    static void TokenizeInstruction(BeaInstructionToken* instr, const DISASM* disasm);
    static void TokenToRichText(const BeaInstructionToken* instr, QList<RichTextPainter::CustomRichText_t>* richTextList);

private:
    //variables
    static QMap<BeaTokenType, BeaTokenColor> colorNamesMap;
    static QStringList segmentNames;
    static QMap<int, QString> memSizeNames;
    static QMap<int, QMap<ARGUMENTS_TYPE, QString>> registerMap;

    //functions
    static void AddToken(BeaInstructionToken* instr, const BeaTokenType type, const QString text, const BeaTokenValue* value);
    static void Prefix(BeaInstructionToken* instr, const DISASM* disasm);
    static bool IsNopInstruction(QString mnemonic, const DISASM* disasm);
    static void Mnemonic(BeaInstructionToken* instr, const DISASM* disasm);
    static QString PrintValue(const BeaTokenValue* value);
    static QString RegisterToString(int size, int reg);
    static void Argument(BeaInstructionToken* instr, const DISASM* disasm, const ARGTYPE* arg, bool* hadarg);
    static void AddColorName(BeaTokenType type, QString color, QString backgroundColor);
};

#endif // BEATOKENIZER_H
