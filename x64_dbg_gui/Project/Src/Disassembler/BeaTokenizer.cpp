#include "BeaTokenizer.h"
#include "Configuration.h"

//variables
QMap<BeaTokenizer::BeaTokenType, BeaTokenizer::BeaTokenColor> BeaTokenizer::colorNamesMap;

void BeaTokenizer::AddColorName(BeaTokenType type, QString color, QString backgroundColor)
{
    BeaTokenColor tokenColor;
    tokenColor.color = color;
    tokenColor.backgroundColor = backgroundColor;
    colorNamesMap.insert(type, tokenColor);
}

void BeaTokenizer::Init()
{
    //color names map
    colorNamesMap.clear();
    //filling
    AddColorName(TokenComma, "InstructionCommaColor", "InstructionCommaBackgroundColor");
    AddColorName(TokenSpace, "", "");
    AddColorName(TokenArgumentSpace, "", "");
    AddColorName(TokenMemoryOperatorSpace, "", "");
    //general instruction parts
    AddColorName(TokenPrefix, "InstructionPrefixColor", "InstructionPrefixBackgroundColor");
    AddColorName(TokenUncategorized, "InstructionUncategorizedColor", "InstructionUncategorizedBackgroundColor");
    AddColorName(TokenAddress, "InstructionAddressColor", "InstructionAddressBackgroundColor"); //jump/call destinations
    AddColorName(TokenValue, "InstructionValueColor", "InstructionValueBackgroundColor");
    //mnemonics
    AddColorName(TokenMnemonicNormal, "InstructionMnemonicColor", "InstructionMnemonicBackgroundColor");
    AddColorName(TokenMnemonicPushPop, "InstructionPushPopColor", "InstructionPushPopBackgroundColor");
    AddColorName(TokenMnemonicCall, "InstructionCallColor", "InstructionCallBackgroundColor");
    AddColorName(TokenMnemonicRet, "InstructionRetColor", "InstructionRetBackgroundColor");
    AddColorName(TokenMnemonicCondJump, "InstructionConditionalJumpColor", "InstructionConditionalJumpBackgroundColor");
    AddColorName(TokenMnemonicUncondJump, "InstructionUnconditionalJumpColor", "InstructionUnconditionalJumpBackgroundColor");
    AddColorName(TokenMnemonicNop, "InstructionNopColor", "InstructionNopBackgroundColor");
    AddColorName(TokenMnemonicFar, "InstructionFarColor", "InstructionFarBackgroundColor");
    AddColorName(TokenMnemonicInt3, "InstructionInt3Color", "InstructionInt3BackgroundColor");
    //memory
    AddColorName(TokenMemorySize, "InstructionMemorySizeColor", "InstructionMemorySizeBackgroundColor");
    AddColorName(TokenMemorySegment, "InstructionMemorySegmentColor", "InstructionMemorySegmentBackgroundColor");
    AddColorName(TokenMemoryBrackets, "InstructionMemoryBracketsColor", "InstructionMemoryBracketsBackgroundColor");
    AddColorName(TokenMemoryStackBrackets, "InstructionMemoryStackBracketsColor", "InstructionMemoryStackBracketsBackgroundColor");
    AddColorName(TokenMemoryBaseRegister, "InstructionMemoryBaseRegisterColor", "InstructionMemoryBaseRegisterBackgroundColor");
    AddColorName(TokenMemoryIndexRegister, "InstructionMemoryIndexRegisterColor", "InstructionMemoryIndexRegisterBackgroundColor");
    AddColorName(TokenMemoryScale, "InstructionMemoryScaleColor", "InstructionMemoryScaleBackgroundColor");
    AddColorName(TokenMemoryOperator, "InstructionMemoryOperatorColor", "InstructionMemoryOperatorBackgroundColor");
    //registers
    AddColorName(TokenGeneralRegister, "InstructionGeneralRegisterColor", "InstructionGeneralRegisterBackgroundColor");
    AddColorName(TokenFpuRegister, "InstructionFpuRegisterColor", "InstructionFpuRegisterBackgroundColor");
    AddColorName(TokenMmxRegister, "InstructionMmxRegisterColor", "InstructionMmxRegisterBackgroundColor");
    AddColorName(TokenSseRegister, "InstructionSseRegisterColor", "InstructionSseRegisterBackgroundColor");
}

void BeaTokenizer::TokenToRichText(const BeaInstructionToken* instr, QList<RichTextPainter::CustomRichText_t>* richTextList, const BeaSingleToken* highlightToken)
{
    QColor highlightColor = ConfigColor("InstructionHighlightColor");
    for(int i = 0; i < instr->tokens.size(); i++)
    {
        BeaSingleToken token = instr->tokens.at(i);
        RichTextPainter::CustomRichText_t richText;
        richText.highlight = TokenEquals(&token, highlightToken);
        richText.highlightColor = highlightColor;
        richText.flags = FlagNone;
        richText.text = token.text;
        if(colorNamesMap.contains(token.type))
        {
            BeaTokenColor tokenColor = colorNamesMap[token.type];
            QString color = tokenColor.color;
            QString backgroundColor = tokenColor.backgroundColor;
            if(color.length() && backgroundColor.length())
            {
                richText.flags = FlagAll;
                richText.textColor = ConfigColor(color);
                richText.textBackground = ConfigColor(backgroundColor);
            }
            else if(color.length())
            {
                richText.flags = FlagColor;
                richText.textColor = ConfigColor(color);
            }
            else if(backgroundColor.length())
            {
                richText.flags = FlagBackground;
                richText.textBackground = ConfigColor(backgroundColor);
            }
        }
        richTextList->append(richText);
    }
}

bool BeaTokenizer::TokenFromX(const BeaInstructionToken* instr, BeaSingleToken* token, int x, int charwidth)
{
    if(x < instr->x) //before the first token
        return false;
    for(int i = 0, xStart = instr->x; i < instr->tokens.size(); i++)
    {
        const BeaSingleToken* curToken = &instr->tokens.at(i);
        int curWidth = curToken->text.length() * charwidth;
        int xEnd = xStart + curWidth;
        if(x >= xStart && x < xEnd)
        {
            *token = *curToken;
            return true;
        }
        xStart = xEnd;
    }
    return false; //not found
}

bool BeaTokenizer::IsHighlightableToken(const BeaSingleToken* token)
{
    switch(token->type)
    {
    case TokenComma:
    case TokenSpace:
    case TokenArgumentSpace:
    case TokenMemoryOperatorSpace:
    case TokenMemoryBrackets:
    case TokenMemoryStackBrackets:
    case TokenMemoryOperator:
        return false;
        break;
    }
    return true;
}

bool BeaTokenizer::TokenEquals(const BeaSingleToken* a, const BeaSingleToken* b, bool ignoreSize)
{
    if(!a || !b)
        return false;
    if(a->value.size != 0 && b->value.size != 0) //we have a value
    {
        if(!ignoreSize && a->value.size != b->value.size)
            return false;
        else if(a->value.value != b->value.value)
            return false;
    }
    else if(a->text != b->text) //text doesn't equal
        return false;
    return true; //passed all checks
}
