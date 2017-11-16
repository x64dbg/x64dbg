#ifndef CS_CAPSTONE_GUI_H
#define CS_CAPSTONE_GUI_H

#include <capstone_wrapper.h>
#include "RichTextPainter.h"
#include "Configuration.h"
#include <map>
#include <QHash>
#include <QtCore>

#include "capstone_gui.h"

class CsCapstoneTokenizer
{
public:
    CsCapstoneTokenizer(int maxModuleLength);
    bool Tokenize(duint addr, const unsigned char* data, int datasize, CapstoneTokenizer::InstructionToken & instruction);
    bool TokenizeData(const QString & datatype, const QString & data, CapstoneTokenizer::InstructionToken & instruction);
    void UpdateConfig();
    void SetConfig(bool bUppercase, bool bTabbedMnemonic, bool bArgumentSpaces, bool bHidePointerSizes, bool bHideNormalSegments, bool bMemorySpaces, bool bNoHighlightOperands, bool bNoCurrentModuleText, bool b0xPrefixValues);
    int Size() const;
    const Capstone & GetCapstone() const;

    static void UpdateColors();
    static void UpdateStringPool();
    static void TokenToRichText(const CapstoneTokenizer::InstructionToken & instr, RichTextPainter::List & richTextList, const CapstoneTokenizer::SingleToken* highlightToken);
    static bool TokenFromX(const CapstoneTokenizer::InstructionToken & instr, CapstoneTokenizer::SingleToken & token, int x, CachedFontMetrics* fontMetrics);
    static bool IsHighlightableToken(const CapstoneTokenizer::SingleToken & token);
    static bool TokenEquals(const CapstoneTokenizer::SingleToken* a, const CapstoneTokenizer::SingleToken* b, bool ignoreSize = true);
    static void addColorName(CapstoneTokenizer::TokenType type, QString color, QString backgroundColor);
    static void addStringsToPool(const QString & regs);
    static bool tokenTextPoolEquals(const QString & a, const QString & b);

private:
    Capstone _cp;
    bool isNop;
    CapstoneTokenizer::InstructionToken _inst;
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
    CapstoneTokenizer::TokenType _mnemonicType;

    void addToken(CapstoneTokenizer::TokenType type, QString text, const CapstoneTokenizer::TokenValue & value);
    void addToken(CapstoneTokenizer::TokenType type, const QString & text);
    void addMemoryOperator(char operatorText);
    QString printValue(const CapstoneTokenizer::TokenValue & value, bool expandModule, int maxModuleLength) const;

    static QHash<QString, int> stringPoolMap;
    static int poolId;

    bool tokenizePrefix();
    bool tokenizeMnemonic();
    bool tokenizeMnemonic(CapstoneTokenizer::TokenType type, const QString & mnemonic);
    bool tokenizeOperand(const cs_x86_op & op);
    bool tokenizeRegOperand(const cs_x86_op & op);
    bool tokenizeImmOperand(const cs_x86_op & op);
    bool tokenizeMemOperand(const cs_x86_op & op);
    bool tokenizeInvalidOperand(const cs_x86_op & op);
};

#endif //CS_CAPSTONE_GUI_H
