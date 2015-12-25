#include "capstone_gui.h"
#include "Configuration.h"
#include "StringUtil.h"

CapstoneTokenizer::CapstoneTokenizer(int maxModuleLength)
    : _maxModuleLength(maxModuleLength),
      _success(false)
{
    SetConfig(false, false, false, false);
}

std::map<CapstoneTokenizer::TokenType, CapstoneTokenizer::TokenColor> CapstoneTokenizer::colorNamesMap;

void CapstoneTokenizer::addColorName(TokenType type, QString color, QString backgroundColor)
{
    colorNamesMap.insert({type, TokenColor(color, backgroundColor)});
}

void CapstoneTokenizer::UpdateColors()
{
    //color names map
    colorNamesMap.clear();
    //filling
    addColorName(TokenType::Comma, "InstructionCommaColor", "InstructionCommaBackgroundColor");
    addColorName(TokenType::Space, "", "");
    addColorName(TokenType::ArgumentSpace, "", "");
    addColorName(TokenType::MemoryOperatorSpace, "", "");
    //general instruction parts
    addColorName(TokenType::Prefix, "InstructionPrefixColor", "InstructionPrefixBackgroundColor");
    addColorName(TokenType::Uncategorized, "InstructionUncategorizedColor", "InstructionUncategorizedBackgroundColor");
    addColorName(TokenType::Address, "InstructionAddressColor", "InstructionAddressBackgroundColor"); //jump/call destinations
    addColorName(TokenType::Value, "InstructionValueColor", "InstructionValueBackgroundColor");
    //mnemonics
    addColorName(TokenType::MnemonicNormal, "InstructionMnemonicColor", "InstructionMnemonicBackgroundColor");
    addColorName(TokenType::MnemonicPushPop, "InstructionPushPopColor", "InstructionPushPopBackgroundColor");
    addColorName(TokenType::MnemonicCall, "InstructionCallColor", "InstructionCallBackgroundColor");
    addColorName(TokenType::MnemonicRet, "InstructionRetColor", "InstructionRetBackgroundColor");
    addColorName(TokenType::MnemonicCondJump, "InstructionConditionalJumpColor", "InstructionConditionalJumpBackgroundColor");
    addColorName(TokenType::MnemonicUncondJump, "InstructionUnconditionalJumpColor", "InstructionUnconditionalJumpBackgroundColor");
    addColorName(TokenType::MnemonicNop, "InstructionNopColor", "InstructionNopBackgroundColor");
    addColorName(TokenType::MnemonicFar, "InstructionFarColor", "InstructionFarBackgroundColor");
    addColorName(TokenType::MnemonicInt3, "InstructionInt3Color", "InstructionInt3BackgroundColor");
    //memory
    addColorName(TokenType::MemorySize, "InstructionMemorySizeColor", "InstructionMemorySizeBackgroundColor");
    addColorName(TokenType::MemorySegment, "InstructionMemorySegmentColor", "InstructionMemorySegmentBackgroundColor");
    addColorName(TokenType::MemoryBrackets, "InstructionMemoryBracketsColor", "InstructionMemoryBracketsBackgroundColor");
    addColorName(TokenType::MemoryStackBrackets, "InstructionMemoryStackBracketsColor", "InstructionMemoryStackBracketsBackgroundColor");
    addColorName(TokenType::MemoryBaseRegister, "InstructionMemoryBaseRegisterColor", "InstructionMemoryBaseRegisterBackgroundColor");
    addColorName(TokenType::MemoryIndexRegister, "InstructionMemoryIndexRegisterColor", "InstructionMemoryIndexRegisterBackgroundColor");
    addColorName(TokenType::MemoryScale, "InstructionMemoryScaleColor", "InstructionMemoryScaleBackgroundColor");
    addColorName(TokenType::MemoryOperator, "InstructionMemoryOperatorColor", "InstructionMemoryOperatorBackgroundColor");
    //registers
    addColorName(TokenType::GeneralRegister, "InstructionGeneralRegisterColor", "InstructionGeneralRegisterBackgroundColor");
    addColorName(TokenType::FpuRegister, "InstructionFpuRegisterColor", "InstructionFpuRegisterBackgroundColor");
    addColorName(TokenType::MmxRegister, "InstructionMmxRegisterColor", "InstructionMmxRegisterBackgroundColor");
    addColorName(TokenType::XmmRegister, "InstructionXmmRegisterColor", "InstructionXmmRegisterBackgroundColor");
    addColorName(TokenType::YmmRegister, "InstructionYmmRegisterColor", "InstructionYmmRegisterBackgroundColor");
    addColorName(TokenType::ZmmRegister, "InstructionZmmRegisterColor", "InstructionZmmRegisterBackgroundColor");
}

bool CapstoneTokenizer::Tokenize(duint addr, const unsigned char* data, int datasize, InstructionToken & instruction)
{
    _inst = InstructionToken();

    _success = _cp.Disassemble(addr, data, datasize);
    if(_success)
    {
        if(!tokenizeMnemonic())
            return false;

        for(int i = 0; i < _cp.OpCount(); i++)
        {
            if(i)
            {
                addToken(TokenType::Comma, ",");
                if(_bArgumentSpaces)
                    addToken(TokenType::ArgumentSpace, " ");
            }
            if(!tokenizeOperand(_cp[i]))
                return false;
        }
    }
    else
        addToken(TokenType::Uncategorized, "???");

    instruction = _inst;

    return true;
}

void CapstoneTokenizer::UpdateConfig()
{
    SetConfig(ConfigBool("Disassembler", "Uppercase"),
              ConfigBool("Disassembler", "TabbedMnemonic"),
              ConfigBool("Disassembler", "ArgumentSpaces"),
              ConfigBool("Disassembler", "MemorySpaces"));
}

void CapstoneTokenizer::SetConfig(bool bUppercase, bool bTabbedMnemonic, bool bArgumentSpaces, bool bMemorySpaces)
{
    _bUppercase = bUppercase;
    _bTabbedMnemonic = bTabbedMnemonic;
    _bArgumentSpaces = bArgumentSpaces;
    _bMemorySpaces = bMemorySpaces;
}

int CapstoneTokenizer::Size() const
{
    return _success ? _cp.Size() : 1;
}

const Capstone & CapstoneTokenizer::GetCapstone() const
{
    return _cp;
}

void CapstoneTokenizer::TokenToRichText(const InstructionToken & instr, QList<RichTextPainter::CustomRichText_t> & richTextList, const SingleToken* highlightToken)
{
    QColor highlightColor = ConfigColor("InstructionHighlightColor");
    for(const auto & token : instr.tokens)
    {
        RichTextPainter::CustomRichText_t richText;
        richText.highlight = TokenEquals(&token, highlightToken);
        richText.highlightColor = highlightColor;
        richText.flags = RichTextPainter::FlagNone;
        richText.text = token.text;
        auto found = colorNamesMap.find(token.type);
        if(found != colorNamesMap.end())
        {
            auto tokenColor = found->second;
            richText.flags = tokenColor.flags;
            richText.textColor = tokenColor.color;
            richText.textBackground = tokenColor.backgroundColor;
        }
        richTextList.append(richText);
    }
}

bool CapstoneTokenizer::TokenFromX(const InstructionToken & instr, SingleToken & token, int x, int charwidth)
{
    if(x < instr.x) //before the first token
        return false;
    int len = int(instr.tokens.size());
    for(int i = 0, xStart = instr.x; i < len; i++)
    {
        const auto & curToken = instr.tokens.at(i);
        int curWidth = int(curToken.text.length()) * charwidth;
        int xEnd = xStart + curWidth;
        if(x >= xStart && x < xEnd)
        {
            token = curToken;
            return true;
        }
        xStart = xEnd;
    }
    return false; //not found
}

bool CapstoneTokenizer::IsHighlightableToken(const SingleToken & token)
{
    switch(token.type)
    {
    case TokenType::Comma:
    case TokenType::Space:
    case TokenType::ArgumentSpace:
    case TokenType::MemoryOperatorSpace:
    case TokenType::MemoryBrackets:
    case TokenType::MemoryStackBrackets:
    case TokenType::MemoryOperator:
        return false;
        break;
    }
    return true;
}

bool CapstoneTokenizer::TokenEquals(const SingleToken* a, const SingleToken* b, bool ignoreSize)
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

void CapstoneTokenizer::addToken(TokenType type, QString text, const TokenValue & value)
{
    switch(type)
    {
    case TokenType::Space:
    case TokenType::ArgumentSpace:
    case TokenType::MemoryOperatorSpace:
        break;
    default:
        text = text.trimmed();
        break;
    }
    if(_bUppercase && !value.size)
        text = text.toUpper();
    _inst.tokens.push_back(SingleToken(type, text, value));
}

void CapstoneTokenizer::addToken(TokenType type, const QString & text)
{
    addToken(type, text, TokenValue());
}

void CapstoneTokenizer::addMemoryOperator(char operatorText)
{
    if(_bMemorySpaces)
        addToken(TokenType::MemoryOperatorSpace, " ");
    QString text;
    text += operatorText;
    addToken(TokenType::MemoryOperator, text);
    if(_bMemorySpaces)
        addToken(TokenType::MemoryOperatorSpace, " ");
}

QString CapstoneTokenizer::printValue(const TokenValue & value, bool expandModule, int maxModuleLength) const
{
    QString labelText;
    char label_[MAX_LABEL_SIZE] = "";
    char module_[MAX_MODULE_SIZE] = "";
    QString moduleText;
    duint addr = value.value;
    bool bHasLabel = DbgGetLabelAt(addr, SEG_DEFAULT, label_);
    if(!bHasLabel) //handle function+offset
    {
        duint start;
        if(DbgFunctionGet(addr, &start, nullptr) && DbgGetLabelAt(start, SEG_DEFAULT, label_))
        {
            labelText = QString("%1+%2").arg(label_).arg(ToHexString(addr - start));
            bHasLabel = true;
        }
    }
    else
        labelText = QString(label_);
    bool bHasModule = (expandModule && DbgGetModuleAt(addr, module_) && !QString(labelText).startsWith("JMP.&"));
    moduleText = QString(module_);
    if(maxModuleLength != -1)
        moduleText.truncate(maxModuleLength);
    if(moduleText.length())
        moduleText += ".";
    QString addrText = ToHexString(addr);
    QString finalText;
    if(bHasLabel && bHasModule)  //<module.label>
        finalText = QString("<%1%2>").arg(moduleText).arg(labelText);
    else if(bHasModule)  //module.addr
        finalText = QString("%1%2").arg(moduleText).arg(addrText);
    else if(bHasLabel)  //<label>
        finalText = QString("<%1>").arg(labelText);
    else
        finalText = addrText;
    return finalText;
}

bool CapstoneTokenizer::tokenizePrefix()
{
    bool hasPrefix = true;
    QString prefixText;
    //TODO: look at multiple prefixes on one instruction (https://github.com/aquynh/capstone/blob/921904888d7c1547c558db3a24fa64bcf97dede4/arch/X86/X86DisassemblerDecoder.c#L540)
    switch(_cp.x86().prefix[0])
    {
    case X86_PREFIX_LOCK:
        prefixText = "lock";
        break;
    case X86_PREFIX_REP:
        prefixText = "rep";
        break;
    case X86_PREFIX_REPNE:
        prefixText = "repne";
        break;
    default:
        hasPrefix = false;
    }

    if(hasPrefix)
    {
        addToken(TokenType::Prefix, prefixText);
        addToken(TokenType::Space, " ");
    }

    return true;
}

bool CapstoneTokenizer::tokenizeMnemonic()
{
    auto type = TokenType::MnemonicNormal;
    auto id = _cp.GetId();
    if(_cp.InGroup(CS_GRP_CALL))
        type = TokenType::MnemonicCall;
    else if(_cp.InGroup(CS_GRP_RET))
        type = TokenType::MnemonicRet;
    else if(_cp.InGroup(CS_GRP_JUMP) || _cp.IsLoop())
    {
        switch(id)
        {
        case X86_INS_JMP:
        case X86_INS_LOOP:
            type = TokenType::MnemonicUncondJump;
            break;
        default:
            type = TokenType::MnemonicCondJump;
            break;
        }
    }
    else if(_cp.IsNop())
        type = TokenType::MnemonicNop;
    else if(_cp.IsInt3())
        type = TokenType::MnemonicInt3;
    else
    {
        switch(id)
        {
        case X86_INS_PUSH:
        case X86_INS_POP:
            type = TokenType::MnemonicPushPop;
            break;
        default:
            break;
        }
    }
    QString mnemonic = QString(_cp.Mnemonic().c_str());
    addToken(type, mnemonic);
    if(_bTabbedMnemonic)
    {
        int spaceCount = 7 - mnemonic.length();
        if(spaceCount > 0)
        {
            for(int i = 0; i < spaceCount; i++)
                addToken(TokenType::Space, " ");
        }
    }
    addToken(TokenType::Space, " ");
    return true;
}

bool CapstoneTokenizer::tokenizeOperand(const cs_x86_op & op)
{
    switch(op.type)
    {
    case X86_OP_REG:
        return tokenizeRegOperand(op);
    case X86_OP_IMM:
        return tokenizeImmOperand(op);
    case X86_OP_MEM:
        return tokenizeMemOperand(op);
    case X86_OP_FP:
        return tokenizeFpOperand(op);
    case X86_OP_INVALID:
        return tokenizeInvalidOperand(op);
    default:
        return false;
    }
}

bool CapstoneTokenizer::tokenizeRegOperand(const cs_x86_op & op)
{
    auto registerType = TokenType::GeneralRegister;
    auto reg = op.reg;
    if(reg >= X86_REG_FP0 && reg <= X86_REG_FP7)
        registerType = TokenType::FpuRegister;
    else if(reg >= X86_REG_ST0 && reg <= X86_REG_ST7)
        registerType = TokenType::FpuRegister;
    else if(reg >= X86_REG_MM0 && reg <= X86_REG_MM7)
        registerType = TokenType::MmxRegister;
    else if(reg >= X86_REG_XMM0 && reg <= X86_REG_XMM31)
        registerType = TokenType::XmmRegister;
    else if(reg >= X86_REG_YMM0 && reg <= X86_REG_YMM31)
        registerType = TokenType::YmmRegister;
    else if(reg >= X86_REG_ZMM0 && reg <= X86_REG_ZMM31)
        registerType = TokenType::ZmmRegister;
    addToken(registerType, _cp.RegName(x86_reg(reg)));
    return true;
}

bool CapstoneTokenizer::tokenizeImmOperand(const cs_x86_op & op)
{
    duint value = duint(op.imm);
    auto valueType = TokenType::Value;
    if(_cp.InGroup(CS_GRP_JUMP) || _cp.InGroup(CS_GRP_CALL) || _cp.IsLoop())
    {
        valueType = TokenType::Address;
    }
    auto tokenValue = TokenValue(op.size, value);
    addToken(valueType, printValue(tokenValue, true, _maxModuleLength), tokenValue);
    return true;
}

bool CapstoneTokenizer::tokenizeMemOperand(const cs_x86_op & op)
{
    //memory size
    const char* sizeText = _cp.MemSizeName(op.size);
    if(!sizeText)
        return false;
    addToken(TokenType::MemorySize, QString(sizeText) + " ptr");
    addToken(TokenType::Space, " ");

    //memory segment
    const auto & mem = op.mem;
    const char* segmentText = _cp.RegName(x86_reg(mem.segment));
    if(mem.segment == X86_REG_INVALID) //segment not set
    {
        switch(x86_reg(mem.base))
        {
        case X86_REG_ESP:
        case X86_REG_RSP:
        case X86_REG_EBP:
        case X86_REG_RBP:
            segmentText = "ss";
            break;
        default:
            segmentText = "ds";
            break;
        }
    }
    addToken(TokenType::MemorySegment, segmentText);
    addToken(TokenType::Uncategorized, ":");

    //memory opening bracket
    auto bracketsType = TokenType::MemoryBrackets;
    switch(x86_reg(mem.base))
    {
    case X86_REG_ESP:
    case X86_REG_RSP:
    case X86_REG_EBP:
    case X86_REG_RBP:
        bracketsType = TokenType::MemoryStackBrackets;
    default:
        break;
    }
    addToken(bracketsType, "[");

    //stuff inside the brackets
    if(mem.base == X86_REG_RIP)   //rip-relative (#replacement)
    {
        duint addr = _cp.Address() + duint(mem.disp) + _cp.Size();
        TokenValue value = TokenValue(op.size, addr);
        auto displacementType = DbgMemIsValidReadPtr(addr) ? TokenType::Address : TokenType::Value;
        addToken(displacementType, printValue(value, false, _maxModuleLength), value);
    }
    else //#base + #index * #scale + #displacement
    {
        bool prependPlus = false;
        if(mem.base != X86_REG_INVALID)  //base register
        {
            addToken(TokenType::MemoryBaseRegister, _cp.RegName(x86_reg(mem.base)));
            prependPlus = true;
        }
        if(mem.index != X86_REG_INVALID)  //index register
        {
            if(prependPlus)
                addMemoryOperator('+');
            addToken(TokenType::MemoryIndexRegister, _cp.RegName(x86_reg(mem.index)));
            if(mem.scale > 1)
            {
                addMemoryOperator('*');
                addToken(TokenType::MemoryScale, QString().sprintf("%d", mem.scale));
            }
            prependPlus = true;
        }
        if(mem.disp)
        {
            char operatorText = '+';
            TokenValue value(op.size, duint(mem.disp));
            auto displacementType = DbgMemIsValidReadPtr(duint(mem.disp)) ? TokenType::Address : TokenType::Value;
            QString valueText;
            if(mem.disp < 0)
            {
                operatorText = '-';
                valueText = printValue(TokenValue(op.size, duint(mem.disp * -1)), false, _maxModuleLength);
            }
            else
                valueText = printValue(value, false, _maxModuleLength);
            if(prependPlus)
                addMemoryOperator(operatorText);
            addToken(displacementType, valueText, value);
        }
    }

    //closing bracket
    addToken(bracketsType, "]");

    return true;
}

bool CapstoneTokenizer::tokenizeFpOperand(const cs_x86_op & op)
{
    addToken(TokenType::Uncategorized, QString().sprintf("%f", op.fp));
    return true;
}

bool CapstoneTokenizer::tokenizeInvalidOperand(const cs_x86_op & op)
{
    addToken(TokenType::Uncategorized, "???");
    return true;
}
