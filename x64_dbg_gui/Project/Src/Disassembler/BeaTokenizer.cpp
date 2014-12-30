#include "BeaTokenizer.h"
#include "Configuration.h"

//variables
QMap<BeaTokenizer::BeaTokenType, BeaTokenizer::BeaTokenColor> BeaTokenizer::colorNamesMap;
QStringList BeaTokenizer::segmentNames;
QMap<int, QString> BeaTokenizer::memSizeNames;
QMap<int, QMap<ARGUMENTS_TYPE, QString>> BeaTokenizer::registerMap;
QSet<int> BeaTokenizer::repSet;

//functions
BeaTokenizer::BeaTokenizer()
{
}

void BeaTokenizer::AddToken(BeaInstructionToken* instr, const BeaTokenType type, const QString text, const BeaTokenValue* value)
{
    BeaSingleToken token;
    token.type = type;
    if(type != TokenSpace && type != TokenArgumentSpace && type != TokenMemoryOperatorSpace)
        token.text = text.trimmed(); //remove whitespaces from the start and end
    else
        token.text = text;
    if(value)
        token.value = *value;
    else
    {
        if(ConfigBool("Disassembler", "Uppercase"))
            token.text = token.text.toUpper();
        token.value.size = 0;
        token.value.value = 0;
    }
    instr->tokens.push_back(token);
}

void BeaTokenizer::Prefix(BeaInstructionToken* instr, const DISASM* disasm)
{
    if(disasm->Prefix.LockPrefix)
    {
        AddToken(instr, TokenPrefix, "lock", 0);
        AddToken(instr, TokenSpace, " ", 0);
    }
    if(disasm->Prefix.RepPrefix)
    {
        if(repSet.contains(disasm->Instruction.Opcode))
        {
            AddToken(instr, TokenPrefix, "rep", 0);
            AddToken(instr, TokenSpace, " ", 0);
        }
    }
    if(disasm->Prefix.RepnePrefix)
    {
        if(repSet.contains(disasm->Instruction.Opcode))
        {
            AddToken(instr, TokenPrefix, "repne", 0);
            AddToken(instr, TokenSpace, " ", 0);
        }
    }
}

bool BeaTokenizer::IsNopInstruction(QString mnemonic, const DISASM* disasm)
{
    Q_UNUSED(disasm);
    //TODO: add instructions like "mov eax,eax" and "xchg ebx,ebx" and "lea eax,[eax]"
    if(mnemonic == "nop")
        return true;
    return false;
}

void BeaTokenizer::StringInstructionMemory(BeaInstructionToken* instr, int size, QString segment, ARGUMENTS_TYPE reg)
{
    if(memSizeNames.contains(size))
        AddToken(instr, TokenMemorySize, memSizeNames.find(size).value() + " ptr", 0);
    else
        AddToken(instr, TokenMemorySize, "??? ptr", 0);
    AddToken(instr, TokenSpace, " ", 0);
    AddToken(instr, TokenMemorySegment, segment, 0);
    AddToken(instr, TokenUncategorized, ":", 0);
    AddToken(instr, TokenMemoryBrackets, "[", 0);
    AddToken(instr, TokenMemoryBaseRegister, RegisterToString(sizeof(int_t) * 8, reg), 0); //EDI/RDI
    AddToken(instr, TokenMemoryBrackets, "]", 0);
}

void BeaTokenizer::StringInstruction(QString mnemonic, BeaInstructionToken* instr, const DISASM* disasm)
{
    AddToken(instr, TokenMnemonicNormal, mnemonic, 0);
    AddToken(instr, TokenSpace, " ", 0);
    if(mnemonic == "movs")
    {
        StringInstructionMemory(instr, disasm->Argument1.ArgSize, "es", REG7); //EDI/RDI
        AddToken(instr, TokenComma, ",", 0);
        AddToken(instr, TokenArgumentSpace, " ", 0);
        StringInstructionMemory(instr, disasm->Argument1.ArgSize, "ds", REG6); //ESI/RSI
    }
    else if(mnemonic == "cmps")
    {
        StringInstructionMemory(instr, disasm->Argument1.ArgSize, "ds", REG6); //EDI/RDI
        AddToken(instr, TokenComma, ",", 0);
        AddToken(instr, TokenArgumentSpace, " ", 0);
        StringInstructionMemory(instr, disasm->Argument1.ArgSize, "es", REG7); //ESI/RSI
    }
    else if(mnemonic == "scas" || mnemonic == "stos")
        StringInstructionMemory(instr, disasm->Argument1.ArgSize, "es", REG7); //ESI/RSI
    else if(mnemonic == "lods")
        StringInstructionMemory(instr, disasm->Argument1.ArgSize, "ds", REG6); //ESI/RSI
    else if(mnemonic == "outs")
    {
        AddToken(instr, TokenGeneralRegister, "dx", 0);
        AddToken(instr, TokenComma, ",", 0);
        AddToken(instr, TokenArgumentSpace, " ", 0);
        StringInstructionMemory(instr, disasm->Argument2.ArgSize, "es", REG6); //EDI/RDI
    }
}

void BeaTokenizer::Mnemonic(BeaInstructionToken* instr, const DISASM* disasm)
{
    QString mnemonic = QString(disasm->Instruction.Mnemonic).trimmed().toLower();
    bool farMnemonic = mnemonic.contains(" far");
    if(farMnemonic)
        mnemonic.truncate(mnemonic.indexOf(" far"));
    QString completeInstr = QString(disasm->CompleteInstr).trimmed().toLower();
    BeaTokenType type = TokenMnemonicNormal;
    int brtype = disasm->Instruction.BranchType;
    if(brtype) //jump/call
    {
        if(brtype == CallType)
            type = TokenMnemonicCall;
        else if(brtype == RetType)
            type = TokenMnemonicRet;
        else if(brtype == JmpType)
            type = TokenMnemonicUncondJump;
        else //cond jump
            type = TokenMnemonicCondJump;
    }
    else if(mnemonic == "push" || mnemonic == "pop")
        type = TokenMnemonicPushPop;
    else if(IsNopInstruction(mnemonic, disasm)) //nop instructions
        type = TokenMnemonicNop;
    else if(mnemonic == "int3" || mnemonic == "int 3") //int3 instruction on request
        type = TokenMnemonicInt3;
    else if(completeInstr.contains("movs") || completeInstr.contains("cmps") || completeInstr.contains("scas") || completeInstr.contains("lods") || completeInstr.contains("stos") || completeInstr.contains("outs"))
    {
        completeInstr = completeInstr.replace("rep ", "").replace("repne ", "");
        if(completeInstr.length() != 5)
        {
            AddToken(instr, type, mnemonic, 0);
            return;
        }
        if(mnemonic[4] == 'b' || mnemonic[4] == 'w' || mnemonic[4] == 'd' || mnemonic[4] == 'q')
        {
            mnemonic.truncate(4);
            StringInstruction(mnemonic, instr, disasm);
        }
        else
            AddToken(instr, type, mnemonic, 0);
        return;
    }
    AddToken(instr, type, mnemonic, 0);
    if(farMnemonic)
    {
        AddToken(instr, TokenSpace, " ", 0);
        AddToken(instr, TokenMnemonicFar, "far", 0);
    }
}

QString BeaTokenizer::PrintValue(const BeaTokenValue* value, bool module)
{
    char labelText[MAX_LABEL_SIZE] = "";
    char moduleText[MAX_MODULE_SIZE] = "";
    int_t addr = value->value;
    bool bHasLabel = DbgGetLabelAt(addr, SEG_DEFAULT, labelText);
    bool bHasModule = (module && DbgGetModuleAt(addr, moduleText) && !QString(labelText).startsWith("JMP.&"));
    QString addrText;
    addrText = QString("%1").arg(addr & (uint_t) - 1, 0, 16, QChar('0')).toUpper();
    QString finalText;
    if(bHasLabel && bHasModule) //<module.label>
        finalText = QString("<%1.%2>").arg(moduleText).arg(labelText);
    else if(bHasModule) //module.addr
        finalText = QString("%1.%2").arg(moduleText).arg(addrText);
    else if(bHasLabel) //<label>
        finalText = QString("<%1>").arg(labelText);
    else
        finalText = addrText;
    return finalText;
}

QString BeaTokenizer::RegisterToString(int size, int reg)
{
    ARGUMENTS_TYPE regValue = (ARGUMENTS_TYPE)(reg & 0xFFFF);
    if(!registerMap.contains(size)) //invalid size
        return QString("UNKNOWN_REGISTER(size:%1)").arg(size);
    QMap<ARGUMENTS_TYPE, QString>* currentMap = &registerMap.find(size).value();
    if(!currentMap->contains(regValue))
        return QString("UNKNOWN_REGISTER(reg:%1)").arg(reg);
    return currentMap->find(regValue).value();
}

void BeaTokenizer::Argument(BeaInstructionToken* instr, const DISASM* disasm, const ARGTYPE* arg, bool* hadarg)
{
    if(arg->ArgType == NO_ARGUMENT || !arg->ArgMnemonic[0]) //empty/implicit argument
        return;

    if(*hadarg) //there already was an argument before this one
    {
        AddToken(instr, TokenComma, ",", 0);
        AddToken(instr, TokenArgumentSpace, " ", 0);
    }
    else //no arg yet (only prefix/mnemonic
        AddToken(instr, TokenSpace, " ", 0);

    //print argument
    QString argMnemonic = QString(arg->ArgMnemonic).toLower().trimmed();
    if((arg->ArgType & MEMORY_TYPE) == MEMORY_TYPE && QString(disasm->CompleteInstr).contains('[')) //memory argument
    {
        //#size ptr #segment:[#BaseRegister + #IndexRegister*#Scale +/- #Displacement]
        if(memSizeNames.contains(arg->ArgSize))
            AddToken(instr, TokenMemorySize, memSizeNames.find(arg->ArgSize).value() + " ptr", 0);
        else
            AddToken(instr, TokenMemorySize, QString().sprintf("???(%d) ptr", arg->ArgSize), 0);
        AddToken(instr, TokenSpace, " ", 0);
        AddToken(instr, TokenMemorySegment, segmentNames.at(arg->SegmentReg), 0);
        AddToken(instr, TokenUncategorized, ":", 0);

        BeaTokenType bracketsType = TokenMemoryBrackets;
        if((arg->Memory.BaseRegister & REG4) == REG4 || (arg->Memory.BaseRegister & REG5) == REG5) //ESP/EBP
            bracketsType = TokenMemoryStackBrackets;
        AddToken(instr, bracketsType, "[", 0);

        bool prependPlusMinus = false;
        if(arg->Memory.BaseRegister) //base register
        {
            AddToken(instr, TokenMemoryBaseRegister, RegisterToString(sizeof(int_t) * 8, arg->Memory.BaseRegister), 0);
            prependPlusMinus = true;
        }
        if(arg->Memory.IndexRegister) //index register + scale
        {
            if(prependPlusMinus)
            {
                AddToken(instr, TokenMemoryOperatorSpace, " ", 0);
                AddToken(instr, TokenMemoryOperator, "+", 0);
                AddToken(instr, TokenMemoryOperatorSpace, " ", 0);
            }
            AddToken(instr, TokenMemoryIndexRegister, RegisterToString(sizeof(int_t) * 8, arg->Memory.IndexRegister), 0);
            int scale = arg->Memory.Scale;
            if(scale > 1) //eax * 1 = eax
            {
                AddToken(instr, TokenMemoryOperatorSpace, " ", 0);
                AddToken(instr, TokenMemoryOperator, "*", 0);
                AddToken(instr, TokenMemoryOperatorSpace, " ", 0);
                AddToken(instr, TokenMemoryScale, QString("%1").arg(arg->Memory.Scale), 0);
            }
            prependPlusMinus = true;
        }

        //displacement
        BeaTokenValue displacement;
        displacement.size = arg->ArgSize;
        if((arg->ArgType & RELATIVE_) == RELATIVE_) //rip-relative displacement
            displacement.value = disasm->Instruction.AddrValue;
        else //direct displacement
            displacement.value = arg->Memory.Displacement;
        if(displacement.value || !prependPlusMinus) //support dword ptr fs:[0]
        {
            BeaTokenValue printDisplacement = displacement;
            QString plusMinus = "+";
            if(printDisplacement.value < 0) //negative -> '-(displacement*-1)'
            {
                printDisplacement.value *= -1;
                printDisplacement.size = arg->ArgSize / 8;
                plusMinus = "-";
            }
            if(prependPlusMinus)
            {
                AddToken(instr, TokenMemoryOperatorSpace, " ", 0);
                AddToken(instr, TokenMemoryOperator, plusMinus, 0);
                AddToken(instr, TokenMemoryOperatorSpace, " ", 0);
            }
            BeaTokenType type = TokenValue;
            if(DbgMemIsValidReadPtr(displacement.value)) //pointer
                type = TokenAddress;
            AddToken(instr, type, PrintValue(&printDisplacement, false), &displacement);
        }
        AddToken(instr, bracketsType, "]", 0);
    }
    else if(disasm->Instruction.BranchType != 0 && disasm->Instruction.BranchType != RetType && (arg->ArgType & RELATIVE_) == RELATIVE_) //jump/call
    {
        BeaTokenValue value;
        value.size = arg->ArgSize / 8;
        value.value = disasm->Instruction.AddrValue;
        AddToken(instr, TokenAddress, PrintValue(&value, true), &value);
    }
    else if((arg->ArgType & CONSTANT_TYPE) == CONSTANT_TYPE) //immediat
    {
        BeaTokenValue value;
        value.size = arg->ArgSize / 8;
        //nice little hack
        LONGLONG val;
        sscanf_s(arg->ArgMnemonic, "%llX", &val);
        value.value = val;
        /*
        switch(value.size)
        {
        case 1:
            value.value=(char)disasm->Instruction.Immediat;
            break;
        case 2:
            value.value=(short)disasm->Instruction.Immediat;
            break;
        case 4:
            value.value=(int)disasm->Instruction.Immediat;
            break;
        case 8:
            value.value=(long long)disasm->Instruction.Immediat;
            break;
        }
        */
        BeaTokenType type = TokenValue;
        if(DbgMemIsValidReadPtr(value.value)) //pointer
            type = TokenAddress;
        AddToken(instr, type, PrintValue(&value, true), &value);
    }
    else if((arg->ArgType & REGISTER_TYPE) == REGISTER_TYPE) //registers
    {
        BeaTokenType type = TokenGeneralRegister; //general x86/unknown register
        if((arg->ArgType & FPU_REG) == FPU_REG) //floating point register
            type = TokenFpuRegister;
        else if((arg->ArgType & MMX_REG) == MMX_REG) //MMX register
            type = TokenMmxRegister;
        else if((arg->ArgType & SSE_REG) == SSE_REG) //SSE register
            type = TokenSseRegister;
        AddToken(instr, type, argMnemonic, 0);
    }
    else //other
        AddToken(instr, TokenUncategorized, argMnemonic, 0);
    *hadarg = true; //we now added an argument
}

void BeaTokenizer::AddColorName(BeaTokenType type, QString color, QString backgroundColor)
{
    BeaTokenColor tokenColor;
    tokenColor.color = color;
    tokenColor.backgroundColor = backgroundColor;
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
    memSizeNames.insert(48, "fword");
    memSizeNames.insert(64, "qword");
    memSizeNames.insert(80, "tword");
    memSizeNames.insert(128, "dqword");
    memSizeNames.insert(256, "yword");
    memSizeNames.insert(512, "zword");
    segmentNames.clear();
    segmentNames << "??" << "es" << "ds" << "fs" << "gs" << "cs" << "ss";

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

    //valid instructions with the REP prefix
    repSet.clear();
    repSet.insert(0x6C);
    repSet.insert(0x6D);
    repSet.insert(0x6E);
    repSet.insert(0x6F);
    repSet.insert(0xA4);
    repSet.insert(0xA5);
    repSet.insert(0xA6);
    repSet.insert(0xA7);
    repSet.insert(0xAA);
    repSet.insert(0xAB);
    repSet.insert(0xAC);
    repSet.insert(0xAD);
    repSet.insert(0xAE);
    repSet.insert(0xAF);

}

//djb2 (http://www.cse.yorku.ca/~oz/hash.html)
unsigned long BeaTokenizer::HashInstruction(const DISASM* disasm)
{
    const char* str = disasm->CompleteInstr;
    unsigned long hash = 5381;
    int c;
    while(c = *str++)
        hash = ((hash << 5) + hash) + c; /*hash*33+c*/
    return hash;
}

void BeaTokenizer::TokenizeInstruction(BeaInstructionToken* instr, const DISASM* disasm)
{
    //initialization
    instr->hash = HashInstruction(disasm); //hash instruction
    instr->tokens.clear(); //clear token list

    //base instruction
    Prefix(instr, disasm);
    Mnemonic(instr, disasm);

    //arguments
    QString mnemonic = QString(disasm->Instruction.Mnemonic).trimmed();
    if(mnemonic.contains("far") && !QString(disasm->CompleteInstr).contains("[")) //far jumps / calls (not the memory ones)
    {
        unsigned int segment = 0;
        unsigned int address = 0;
        sscanf_s(disasm->Argument1.ArgMnemonic, "%X : %X", &segment, &address);
        AddToken(instr, TokenSpace, QString(" "), 0);
        BeaTokenValue val;
        val.size = 2;
        val.value = segment;
        AddToken(instr, TokenValue, PrintValue(&val, true), &val);
        AddToken(instr, TokenUncategorized, ":", 0);
        val.size = 4;
        val.value = address;
        AddToken(instr, TokenAddress, PrintValue(&val, true), &val);
    }
    else
    {
        bool hadarg = false;
        Argument(instr, disasm, &disasm->Argument1, &hadarg);
        Argument(instr, disasm, &disasm->Argument2, &hadarg);
        Argument(instr, disasm, &disasm->Argument3, &hadarg);
    }

    //remove spaces when needed
    bool bArgumentSpaces = ConfigBool("Disassembler", "ArgumentSpaces");
    bool bMemorySpaces = ConfigBool("Disassembler", "MemorySpaces");
    for(int i = instr->tokens.size() - 1; i > -1; i--)
    {
        if(!bArgumentSpaces && instr->tokens.at(i).type == TokenArgumentSpace)
            instr->tokens.erase(instr->tokens.begin() + i);
        if(!bMemorySpaces && instr->tokens.at(i).type == TokenMemoryOperatorSpace)
            instr->tokens.erase(instr->tokens.begin() + i);
    }
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
    if(a->value.size != 0) //we have a value
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
