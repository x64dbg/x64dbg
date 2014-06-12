#include "BeaTokenizer.h"

BeaTokenizer::BeaTokenizer()
{
}

//djb2 (http://www.cse.yorku.ca/~oz/hash.html)
unsigned long BeaTokenizer::BeaHashInstruction(const DISASM* disasm)
{
    const char* str=disasm->CompleteInstr;
    unsigned long hash=5381;
    int c;
    while(c=*str++)
        hash=((hash<<5)+hash)+c; /*hash*33+c*/
    return hash;
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
    QMap<ARGUMENTS_TYPE, int> registerValueMap;
    registerValueMap.insert(REG0, 0);
    registerValueMap.insert(REG1, 1);
    registerValueMap.insert(REG2, 2);
    registerValueMap.insert(REG3, 3);
    registerValueMap.insert(REG4, 4);
    registerValueMap.insert(REG5, 5);
    registerValueMap.insert(REG6, 6);
    registerValueMap.insert(REG7, 7);
    registerValueMap.insert(REG8, 8);
    registerValueMap.insert(REG9, 9);
    registerValueMap.insert(REG10, 10);
    registerValueMap.insert(REG11, 11);
    registerValueMap.insert(REG12, 12);
    registerValueMap.insert(REG13, 13);
    registerValueMap.insert(REG14, 14);
    registerValueMap.insert(REG15, 15);
    if(!registerValueMap.contains(regValue)) //not a register
        return "UNKNOWN_REGISTER";
    QMap<int, const char**> registerMap;
#ifdef _WIN64
    const char Registers8Bits64[16][8] = {"al", "cl", "dl", "bl", "spl", "bpl", "sil", "dil", "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b"};
    registerMap.insert(8, (const char**)Registers8Bits64);
    const char Registers64Bits[16][4] = {"rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"};
    registerMap.insert(64, (const char**)Registers64Bits);
#else //x86
    const char Registers8Bits32[8][4] = {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"};
    registerMap.insert(8, (const char**)Registers8Bits32);
#endif //_WIN64
    const char Registers16Bits[16][8] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di", "r8w", "r9w", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w"};
    registerMap.insert(16, (const char**)Registers16Bits);
    const char Registers32Bits[16][8] = {"eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi", "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d"};
    registerMap.insert(32, (const char**)Registers32Bits);
    if(!registerMap.contains(size)) //invalid size
        return "UNKNOWN_REGISTER";
    return registerMap[size][regValue];
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
        //initialization
        QStringList segmentNames;
        segmentNames<<""<<"es"<<"ds"<<"fs"<<"gs"<<"cs"<<"ss";
        QMap<int, QString> memSizeNames; //lookup table for memory sizes
        memSizeNames.insert(8, "byte");
        memSizeNames.insert(16, "word");
        memSizeNames.insert(32, "dword");
        memSizeNames.insert(64, "qword");
        memSizeNames.insert(80, "tword");
        memSizeNames.insert(128, "dqword");
        memSizeNames.insert(256, "yword");
        memSizeNames.insert(512, "zword");
        //#size ptr #segment:[#BaseRegister + #IndexRegister*#Scale +/- #Displacement]
        AddToken(instr, TokenMemorySize, memSizeNames.find(arg->ArgSize).value(), 0);
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

void BeaTokenizer::BeaTokenizeInstruction(BeaInstructionToken* instr, const DISASM* disasm)
{
    //initialization
    instr->hash=BeaHashInstruction(disasm); //hash instruction
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
