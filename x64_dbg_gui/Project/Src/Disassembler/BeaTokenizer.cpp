#include "BeaTokenizer.h"
#include "Configuration.h"

//variables
QMap<BeaTokenizer::BeaTokenType, BeaTokenizer::BeaTokenColor> BeaTokenizer::colorNamesMap;
QStringList BeaTokenizer::segmentNames;
QMap<int, QString> BeaTokenizer::memSizeNames;
QMap<int, QMap<ARGUMENTS_TYPE, QString>> BeaTokenizer::registerMap;


//functions
BeaTokenizer::BeaTokenizer()
{
}

void BeaTokenizer::AddToken(BeaInstructionToken* instr, const BeaTokenType type, const QString text, const BeaTokenValue* value)
{
    BeaSingleToken token;
    token.type=type;
    if(type!=TokenRequiredSpace && type!=TokenOptionalSpace && type!=TokenMemorySpace)
        token.text=text.trimmed(); //remove whitespaces from the start and end
    else
        token.text=text;
    if(value)
        token.value=*value;
    else
    {
        token.value.size=0;
        token.value.value=0;
    }
    instr->tokens.push_back(token);
}

void BeaTokenizer::Prefix(BeaInstructionToken* instr, const DISASM* disasm)
{
    if(disasm->Prefix.LockPrefix)
    {
        AddToken(instr, TokenPrefix, "lock", 0);
        AddToken(instr, TokenRequiredSpace, " ", 0);
    }
    if(disasm->Prefix.RepPrefix)
    {
        AddToken(instr, TokenPrefix, "rep", 0);
        AddToken(instr, TokenRequiredSpace, " ", 0);
    }
    if(disasm->Prefix.RepnePrefix)
    {
        AddToken(instr, TokenPrefix, "repne", 0);
        AddToken(instr, TokenRequiredSpace, " ", 0);
    }
}

bool BeaTokenizer::IsNopInstruction(QString mnemonic, const DISASM* disasm)
{
    Q_UNUSED(disasm);
    //TODO: add instructions like "mov eax,eax" and "xchg ebx,ebx" and "lea eax,[eax]"
    if(mnemonic=="nop")
        return true;
    return false;
}

void BeaTokenizer::Mnemonic(BeaInstructionToken* instr, const DISASM* disasm)
{
    QString mnemonic=QString(disasm->Instruction.Mnemonic).trimmed().toLower();
    BeaTokenType type=TokenMnemonicNormal;
    int brtype=disasm->Instruction.BranchType;
    if(brtype) //jump/call
    {
        if(brtype==RetType || brtype==CallType)
            type=TokenMnemonicCallRet;
        else if(brtype==JmpType)
            type=TokenMnemonicUncondJump;
        else //cond jump
            type=TokenMnemonicCondJump;
    }
    else if(mnemonic=="push" || mnemonic=="pop")
        type=TokenMnemonicPushPop;
    else if(IsNopInstruction(mnemonic, disasm)) //nop instructions
        type=TokenMnemonicNop;
    AddToken(instr, type, mnemonic, 0);
    AddToken(instr, TokenRequiredSpace, " ", 0);
}

QString BeaTokenizer::PrintValue(const BeaTokenValue* value)
{
    return QString("%1").arg(value->value, 0, 16, QChar('0'));
}

QString BeaTokenizer::RegisterToString(int size, int reg)
{
    ARGUMENTS_TYPE regValue=(ARGUMENTS_TYPE)(reg&0xFFFF);
    if(!registerMap.contains(size)) //invalid size
        return "UNKNOWN_REGISTER";
    QMap<ARGUMENTS_TYPE, QString>* currentMap=&registerMap.find(size).value();
    if(!currentMap->contains(regValue))
        return "UNKNOWN_REGISTER";
    return currentMap->find(regValue).value();
}

void BeaTokenizer::Argument(BeaInstructionToken* instr, const DISASM* disasm, const ARGTYPE* arg, bool* hadarg)
{
    if(arg->ArgType==NO_ARGUMENT || !arg->ArgMnemonic[0]) //empty/implicit argument
        return;

    if(*hadarg) //there already was an argument before this one
    {
        AddToken(instr, TokenComma, ",", 0);
        AddToken(instr, TokenOptionalSpace, " ", 0);
    }

    //print argument
    QString argMnemonic=QString(arg->ArgMnemonic).toLower().trimmed();
    if(disasm->Instruction.BranchType != 0) //jump/call
    {
        BeaTokenValue value;
        value.size=arg->ArgSize/16;
        value.value=disasm->Instruction.AddrValue;
        AddToken(instr, TokenCodeDest, PrintValue(&value), &value);
    }
    else if((arg->ArgType&CONSTANT_TYPE)==CONSTANT_TYPE) //immediat
    {
        BeaTokenValue value;
        value.size=arg->ArgSize/16;
        value.value=disasm->Instruction.Immediat;
        AddToken(instr, TokenImmediat, PrintValue(&value), &value);
    }
    else if((arg->ArgType&MEMORY_TYPE)==MEMORY_TYPE) //memory argument
    {
        //#size ptr #segment:[#BaseRegister + #IndexRegister*#Scale +/- #Displacement]
        if(memSizeNames.contains(arg->ArgSize))
            AddToken(instr, TokenMemorySize, memSizeNames.find(arg->ArgSize).value(), 0);
        else
            AddToken(instr, TokenMemorySize, "???", 0);
        AddToken(instr, TokenRequiredSpace, " ", 0);
        AddToken(instr, TokenMemoryText, "ptr", 0);
        AddToken(instr, TokenRequiredSpace, " ", 0);
        AddToken(instr, TokenMemorySegment, segmentNames.at(arg->SegmentReg), 0);
        AddToken(instr, TokenMemoryBrackets, ":[", 0);

        bool prependPlusMinus=false;
        if(arg->Memory.BaseRegister) //base register
        {
            AddToken(instr, TokenMemoryBaseRegister, RegisterToString(arg->ArgSize, arg->Memory.BaseRegister), 0);
            prependPlusMinus=true;
        }
        if(arg->Memory.IndexRegister) //index register + scale
        {
            if(prependPlusMinus)
            {
                AddToken(instr, TokenMemorySpace, " ", 0);
                AddToken(instr, TokenMemoryPlusMinus, "+", 0);
                AddToken(instr, TokenMemorySpace, " ", 0);
            }
            AddToken(instr, TokenMemoryIndexRegister, RegisterToString(arg->ArgSize, arg->Memory.BaseRegister), 0);
            AddToken(instr, TokenMemoryScale, QString("%1").arg(arg->Memory.Displacement), 0);
            prependPlusMinus=true;
        }

        //displacement
        BeaTokenValue displacement;
        displacement.size=arg->ArgSize;
        if((arg->ArgType&RELATIVE_)==RELATIVE_) //rip-relative displacement
            displacement.value=disasm->Instruction.AddrValue;
        else //direct displacement
            displacement.value=arg->Memory.Displacement;
        if(displacement.value)
        {
            BeaTokenValue printDisplacement=displacement;
            QString plusMinus="+";
            if(printDisplacement.value<0) //negative -> '-(displacement*-1)'
            {
                printDisplacement.value*=-1;
                plusMinus="-";
            }
            if(prependPlusMinus)
            {
                AddToken(instr, TokenMemorySpace, " ", 0);
                AddToken(instr, TokenMemoryPlusMinus, plusMinus, 0);
                AddToken(instr, TokenMemorySpace, " ", 0);
            }
            AddToken(instr, TokenMemoryDisplacement, PrintValue(&printDisplacement), &displacement);
        }
        AddToken(instr, TokenMemoryBrackets, "]", 0);
    }
    else if((arg->ArgType&REGISTER_TYPE)==REGISTER_TYPE) //registers
    {
        BeaTokenType type=TokenGeneralRegister; //general x86/unknown register
        if((arg->ArgType&FPU_REG)==FPU_REG) //floating point register
            type=TokenFpuRegister;
        else if((arg->ArgType&MMX_REG)==MMX_REG) //MMX register
            type=TokenMmxRegister;
        else if((arg->ArgType&SSE_REG)==SSE_REG) //SSE register
            type=TokenSseRegister;
        AddToken(instr, type, argMnemonic, 0);
    }
    else //other
        AddToken(instr, TokenGeneral, argMnemonic, 0);
}

void BeaTokenizer::AddColorName(BeaTokenType type, QString color, QString backgroundColor)
{
    BeaTokenColor tokenColor;
    tokenColor.color=color;
    tokenColor.backgroundColor=color;
    colorNamesMap.insert(type, tokenColor);
}

void BeaTokenizer::Init()
{
    registerMap.clear();
#ifdef _WIN64
    QMap<ARGUMENTS_TYPE, QString> Registers8Bits64;
    Registers8Bits64.insert(REG0, "al");
    Registers8Bits64.insert(REG1, "cl");
    Registers8Bits64.insert(REG2, "dl");
    Registers8Bits64.insert(REG3, "bl");
    Registers8Bits64.insert(REG4, "spl");
    Registers8Bits64.insert(REG5, "bpl");
    Registers8Bits64.insert(REG6, "sil");
    Registers8Bits64.insert(REG7, "dil");
    Registers8Bits64.insert(REG8, "r8b");
    Registers8Bits64.insert(REG9, "r9b");
    Registers8Bits64.insert(REG10, "r10b");
    Registers8Bits64.insert(REG11, "r11b");
    Registers8Bits64.insert(REG12, "r12b");
    Registers8Bits64.insert(REG13, "r13b");
    Registers8Bits64.insert(REG14, "r14b");
    Registers8Bits64.insert(REG15, "r15b");
    registerMap.insert(8, Registers8Bits64);
    QMap<ARGUMENTS_TYPE, QString> Registers64Bits;
    Registers64Bits.insert(REG0, "rax");
    Registers64Bits.insert(REG1, "rcx");
    Registers64Bits.insert(REG2, "rdx");
    Registers64Bits.insert(REG3, "rbx");
    Registers64Bits.insert(REG4, "rsp");
    Registers64Bits.insert(REG5, "rbp");
    Registers64Bits.insert(REG6, "rsi");
    Registers64Bits.insert(REG7, "rdi");
    Registers64Bits.insert(REG8, "r8");
    Registers64Bits.insert(REG9, "r9");
    Registers64Bits.insert(REG10, "r10");
    Registers64Bits.insert(REG11, "r11");
    Registers64Bits.insert(REG12, "r12");
    Registers64Bits.insert(REG13, "r13");
    Registers64Bits.insert(REG14, "r14");
    Registers64Bits.insert(REG15, "r15");
    registerMap.insert(64, Registers64Bits);
#else //x86
    QMap<ARGUMENTS_TYPE, QString> Registers8Bits32;
    Registers8Bits32.insert(REG0, "al");
    Registers8Bits32.insert(REG1, "cl");
    Registers8Bits32.insert(REG2, "dl");
    Registers8Bits32.insert(REG3, "bl");
    Registers8Bits32.insert(REG4, "ah");
    Registers8Bits32.insert(REG5, "ch");
    Registers8Bits32.insert(REG6, "dh");
    Registers8Bits32.insert(REG7, "bh");
    registerMap.insert(8, Registers8Bits32);
#endif //_WIN64
    QMap<ARGUMENTS_TYPE, QString> Registers16Bits;
    Registers16Bits.insert(REG0, "ax");
    Registers16Bits.insert(REG1, "cx");
    Registers16Bits.insert(REG2, "dx");
    Registers16Bits.insert(REG3, "bx");
    Registers16Bits.insert(REG4, "sp");
    Registers16Bits.insert(REG5, "bp");
    Registers16Bits.insert(REG6, "si");
    Registers16Bits.insert(REG7, "di");
#ifdef _WIN64
    Registers16Bits.insert(REG8, "r8w");
    Registers16Bits.insert(REG9, "r9w");
    Registers16Bits.insert(REG10, "r10w");
    Registers16Bits.insert(REG11, "r11w");
    Registers16Bits.insert(REG12, "r12w");
    Registers16Bits.insert(REG13, "r13w");
    Registers16Bits.insert(REG14, "r14w");
    Registers16Bits.insert(REG15, "r15w");
#endif //_WIN64
    registerMap.insert(16, Registers16Bits);

    QMap<ARGUMENTS_TYPE, QString> Registers32Bits;
    Registers32Bits.insert(REG0, "eax");
    Registers32Bits.insert(REG1, "ecx");
    Registers32Bits.insert(REG2, "edx");
    Registers32Bits.insert(REG3, "ebx");
    Registers32Bits.insert(REG4, "esp");
    Registers32Bits.insert(REG5, "ebp");
    Registers32Bits.insert(REG6, "esi");
    Registers32Bits.insert(REG7, "edi");
#ifdef _WIN64
    Registers32Bits.insert(REG8, "r8d");
    Registers32Bits.insert(REG9, "r9d");
    Registers32Bits.insert(REG10, "r10d");
    Registers32Bits.insert(REG11, "r11d");
    Registers32Bits.insert(REG12, "r12d");
    Registers32Bits.insert(REG13, "r13d");
    Registers32Bits.insert(REG14, "r14d");
    Registers32Bits.insert(REG15, "r15d");
#endif //_WIN64
    registerMap.insert(32, Registers32Bits);

    //memory parser
    memSizeNames.clear();
    memSizeNames.insert(8, "byte");
    memSizeNames.insert(16, "word");
    memSizeNames.insert(32, "dword");
    memSizeNames.insert(64, "qword");
    memSizeNames.insert(80, "tword");
    memSizeNames.insert(128, "dqword");
    memSizeNames.insert(256, "yword");
    memSizeNames.insert(512, "zword");
    segmentNames.clear();
    segmentNames<<"??"<<"es"<<"ds"<<"fs"<<"gs"<<"cs"<<"ss";

    //color names map
    colorNamesMap.clear();
    //filling
    AddColorName(TokenComma, "", "");
    AddColorName(TokenComma, "", "");
    AddColorName(TokenRequiredSpace, "", "");
    AddColorName(TokenOptionalSpace, "", "");
    AddColorName(TokenMemorySpace, "", "");
    //general instruction parts
    AddColorName(TokenPrefix, "", "");
    AddColorName(TokenGeneral, "", "");
    AddColorName(TokenCodeDest, "", ""); //jump/call destinations
    AddColorName(TokenImmediat, "", "");
    //mnemonics
    AddColorName(TokenMnemonicNormal, "", "");
    AddColorName(TokenMnemonicPushPop, "", "");
    AddColorName(TokenMnemonicCallRet, "", "");
    AddColorName(TokenMnemonicCondJump, "", "");
    AddColorName(TokenMnemonicUncondJump, "", "");
    AddColorName(TokenMnemonicNop, "", "");
    //memory
    AddColorName(TokenMemorySize, "", "");
    AddColorName(TokenMemoryText, "", "");
    AddColorName(TokenMemorySegment, "", "");
    AddColorName(TokenMemoryBrackets, "", "");
    AddColorName(TokenMemoryBaseRegister, "", "");
    AddColorName(TokenMemoryIndexRegister, "", "");
    AddColorName(TokenMemoryScale, "", "");
    AddColorName(TokenMemoryDisplacement, "", "");
    AddColorName(TokenMemoryPlusMinus, "", "");
    //registers
    AddColorName(TokenGeneralRegister, "", "");
    AddColorName(TokenFpuRegister, "", "");
    AddColorName(TokenMmxRegister, "", "");
    AddColorName(TokenSseRegister, "", "");
}

//djb2 (http://www.cse.yorku.ca/~oz/hash.html)
unsigned long BeaTokenizer::HashInstruction(const DISASM* disasm)
{
    const char* str=disasm->CompleteInstr;
    unsigned long hash=5381;
    int c;
    while(c=*str++)
        hash=((hash<<5)+hash)+c; /*hash*33+c*/
    return hash;
}

void BeaTokenizer::TokenizeInstruction(BeaInstructionToken* instr, const DISASM* disasm)
{
    //initialization
    instr->hash=HashInstruction(disasm); //hash instruction
    instr->tokens.clear(); //clear token list

    //base instruction
    Prefix(instr, disasm);
    Mnemonic(instr, disasm);

    //arguments
    bool hadarg=false;
    Argument(instr, disasm, &disasm->Argument1, &hadarg);
    Argument(instr, disasm, &disasm->Argument2, &hadarg);
    Argument(instr, disasm, &disasm->Argument3, &hadarg);
}

void BeaTokenizer::TokenToRichText(const BeaInstructionToken* instr, QList<RichTextPainter::CustomRichText_t>* richTextList)
{
    for(int i=0; i<instr->tokens.size(); i++)
    {
        BeaSingleToken token=instr->tokens.at(i);
        RichTextPainter::CustomRichText_t richText;
        richText.flags=FlagNone;
        richText.text=token.text;
        if(colorNamesMap.contains(token.type))
        {
            BeaTokenColor tokenColor=colorNamesMap[token.type];
            QString color=tokenColor.color;
            QString backgroundColor=tokenColor.backgroundColor;
            if(color.length() && backgroundColor.length())
            {
                richText.flags=FlagAll;
                richText.textColor=ConfigColor(color);
                richText.textBackground=ConfigColor(backgroundColor);
            }
            else if(color.length())
            {
                richText.flags=FlagColor;
                richText.textColor=ConfigColor(color);
            }
            else if(backgroundColor.length())
            {
                richText.flags=FlagBackground;
                richText.textBackground=ConfigColor(backgroundColor);
            }
        }
        richTextList->append(richText);
    }
}
