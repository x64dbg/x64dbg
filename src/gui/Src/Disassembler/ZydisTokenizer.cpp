#include "ZydisTokenizer.h"
#include "Configuration.h"
#include "StringUtil.h"
#include "CachedFontMetrics.h"

ZydisTokenizer::ZydisTokenizer(int maxModuleLength)
    : _maxModuleLength(maxModuleLength),
      _success(false),
      isNop(false),
      _mnemonicType(TokenType::Uncategorized)
{
    SetConfig(false, false, false, false, false, false, false, false, false);
}

static ZydisTokenizer::TokenColor colorNamesMap[size_t(ZydisTokenizer::TokenType::Last)];
QHash<QString, int> ZydisTokenizer::stringPoolMap;
int ZydisTokenizer::poolId = 0;

void ZydisTokenizer::addColorName(TokenType type, QString color, QString backgroundColor)
{
    colorNamesMap[int(type)] = TokenColor(color, backgroundColor);
}

void ZydisTokenizer::addStringsToPool(const QString & strings)
{
    QStringList stringList = strings.split(' ', QString::SkipEmptyParts);
    for(const QString & string : stringList)
        stringPoolMap.insert(string, poolId);
    poolId++;
}

void ZydisTokenizer::UpdateColors()
{
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
    addColorName(TokenType::MnemonicUnusual, "InstructionUnusualColor", "InstructionUnusualBackgroundColor");
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

void ZydisTokenizer::UpdateStringPool()
{
    poolId = 0;
    stringPoolMap.clear();
    // These registers must be in lower case.
    addStringsToPool("rax eax ax al ah");
    addStringsToPool("rbx ebx bx bl bh");
    addStringsToPool("rcx ecx cx cl ch");
    addStringsToPool("rdx edx dx dl dh");
    addStringsToPool("rsi esi si sil");
    addStringsToPool("rdi edi di dil");
    addStringsToPool("rbp ebp bp bpl");
    addStringsToPool("rsp esp sp spl");
    addStringsToPool("r8 r8d r8w r8b");
    addStringsToPool("r9 r9d r9w r9b");
    addStringsToPool("r10 r10d r10w r10b");
    addStringsToPool("r11 r11d r11w r11b");
    addStringsToPool("r12 r12d r12w r12b");
    addStringsToPool("r13 r13d r13w r13b");
    addStringsToPool("r14 r14d r14w r14b");
    addStringsToPool("r15 r15d r15w r15b");
    addStringsToPool("xmm0 ymm0");
    addStringsToPool("xmm1 ymm1");
    addStringsToPool("xmm2 ymm2");
    addStringsToPool("xmm3 ymm3");
    addStringsToPool("xmm4 ymm4");
    addStringsToPool("xmm5 ymm5");
    addStringsToPool("xmm6 ymm6");
    addStringsToPool("xmm7 ymm7");
    addStringsToPool("xmm8 ymm8");
    addStringsToPool("xmm9 ymm9");
    addStringsToPool("xmm10 ymm10");
    addStringsToPool("xmm11 ymm11");
    addStringsToPool("xmm12 ymm12");
    addStringsToPool("xmm13 ymm13");
    addStringsToPool("xmm14 ymm14");
    addStringsToPool("xmm15 ymm15");
}

bool ZydisTokenizer::Tokenize(duint addr, const unsigned char* data, int datasize, InstructionToken & instruction)
{
    _inst = InstructionToken();

    _success = _cp.DisassembleSafe(addr, data, datasize);
    if(_success)
    {
        if(!tokenizePrefix())
            return false;

        isNop = _cp.IsNop();
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
    {
        isNop = false;
        addToken(TokenType::MnemonicUnusual, "???");
    }

    if(_bNoHighlightOperands)
    {
        while(_inst.tokens.size() && _inst.tokens[_inst.tokens.size() - 1].type == TokenType::Space)
            _inst.tokens.pop_back();
        for(SingleToken & token : _inst.tokens)
            token.type = _mnemonicType;
    }

    instruction = _inst;

    return true;
}

bool ZydisTokenizer::TokenizeData(const QString & datatype, const QString & data, InstructionToken & instruction)
{
    _inst = InstructionToken();
    isNop = false;

    if(!tokenizeMnemonic(TokenType::MnemonicNormal, datatype))
        return false;

    addToken(TokenType::Value, data);

    instruction = _inst;

    return true;
}

void ZydisTokenizer::TokenizeTraceRegister(const char* reg, duint oldValue, duint newValue, std::vector<SingleToken> & tokens)
{
    if(tokens.size() > 0)
    {
        tokens.push_back(SingleToken(TokenType::ArgumentSpace, " ", TokenValue()));
    }
    QString regName(reg);
    tokens.push_back(SingleToken(TokenType::GeneralRegister, ConfigBool("Disassembler", "Uppercase") ? regName.toUpper() : regName, TokenValue()));
    tokens.push_back(SingleToken(TokenType::ArgumentSpace, ": ", TokenValue()));
    tokens.push_back(SingleToken(TokenType::Value, ToHexString(oldValue), TokenValue(8, oldValue)));
    tokens.push_back(SingleToken(TokenType::ArgumentSpace, "-> ", TokenValue()));
    tokens.push_back(SingleToken(TokenType::Value, ToHexString(newValue), TokenValue(8, newValue)));
}

void ZydisTokenizer::TokenizeTraceMemory(duint address, duint oldValue, duint newValue, std::vector<SingleToken> & tokens)
{
    if(tokens.size() > 0)
    {
        tokens.push_back(SingleToken(TokenType::ArgumentSpace, " ", TokenValue()));
    }
    tokens.push_back(SingleToken(TokenType::Address, ToPtrString(address), TokenValue(8, address)));
    tokens.push_back(SingleToken(TokenType::ArgumentSpace, ": ", TokenValue()));
    tokens.push_back(SingleToken(TokenType::Value, ToHexString(oldValue), TokenValue(8, oldValue)));
    tokens.push_back(SingleToken(TokenType::ArgumentSpace, "-> ", TokenValue()));
    tokens.push_back(SingleToken(TokenType::Value, ToHexString(newValue), TokenValue(8, newValue)));
}

void ZydisTokenizer::UpdateConfig()
{
    SetConfig(ConfigBool("Disassembler", "Uppercase"),
              ConfigBool("Disassembler", "TabbedMnemonic"),
              ConfigBool("Disassembler", "ArgumentSpaces"),
              ConfigBool("Disassembler", "HidePointerSizes"),
              ConfigBool("Disassembler", "HideNormalSegments"),
              ConfigBool("Disassembler", "MemorySpaces"),
              ConfigBool("Disassembler", "NoHighlightOperands"),
              ConfigBool("Disassembler", "NoCurrentModuleText"),
              ConfigBool("Disassembler", "0xPrefixValues"));
    _maxModuleLength = (int)ConfigUint("Disassembler", "MaxModuleSize");
    UpdateStringPool();
}

void ZydisTokenizer::SetConfig(bool bUppercase, bool bTabbedMnemonic, bool bArgumentSpaces, bool bHidePointerSizes, bool bHideNormalSegments, bool bMemorySpaces, bool bNoHighlightOperands, bool bNoCurrentModuleText, bool b0xPrefixValues)
{
    _bUppercase = bUppercase;
    _bTabbedMnemonic = bTabbedMnemonic;
    _bArgumentSpaces = bArgumentSpaces;
    _bHidePointerSizes = bHidePointerSizes;
    _bHideNormalSegments = bHideNormalSegments;
    _bMemorySpaces = bMemorySpaces;
    _bNoHighlightOperands = bNoHighlightOperands;
    _bNoCurrentModuleText = bNoCurrentModuleText;
    _b0xPrefixValues = b0xPrefixValues;
}

int ZydisTokenizer::Size() const
{
    return _success ? _cp.Size() : 1;
}

const Zydis & ZydisTokenizer::GetZydis() const
{
    return _cp;
}

void ZydisTokenizer::TokenToRichText(const InstructionToken & instr, RichTextPainter::List & richTextList, const SingleToken* highlightToken)
{
    QColor highlightColor = ConfigColor("InstructionHighlightColor");
    for(const auto & token : instr.tokens)
    {
        RichTextPainter::CustomRichText_t richText;
        richText.highlight = TokenEquals(&token, highlightToken);
        richText.highlightColor = highlightColor;
        richText.flags = RichTextPainter::FlagNone;
        richText.text = token.text;
        if(token.type < TokenType::Last)
        {
            const auto & tokenColor = colorNamesMap[int(token.type)];
            richText.flags = tokenColor.flags;
            richText.textColor = tokenColor.color;
            richText.textBackground = tokenColor.backgroundColor;
        }
        richTextList.push_back(richText);
    }
}

bool ZydisTokenizer::TokenFromX(const InstructionToken & instr, SingleToken & token, int x, CachedFontMetrics* fontMetrics)
{
    if(x < instr.x) //before the first token
        return false;
    int len = int(instr.tokens.size());
    for(int i = 0, xStart = instr.x; i < len; i++)
    {
        const auto & curToken = instr.tokens.at(i);
        int curWidth = fontMetrics->width(curToken.text);
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

bool ZydisTokenizer::IsHighlightableToken(const SingleToken & token)
{
    switch(token.type)
    {
    case TokenType::Comma:
    case TokenType::Space:
    case TokenType::ArgumentSpace:
    case TokenType::Uncategorized:
    case TokenType::MemoryOperatorSpace:
    case TokenType::MemoryBrackets:
    case TokenType::MemoryStackBrackets:
    case TokenType::MemoryOperator:
        return false;
        break;
    }
    return true;
}

bool ZydisTokenizer::tokenTextPoolEquals(const QString & a, const QString & b)
{
    if(a.compare(b, Qt::CaseInsensitive) == 0)
        return true;
    auto found1 = stringPoolMap.find(a.toLower());
    auto found2 = stringPoolMap.find(b.toLower());
    if(found1 == stringPoolMap.end() || found2 == stringPoolMap.end())
        return false;
    return found1.value() == found2.value();
}

bool ZydisTokenizer::TokenEquals(const SingleToken* a, const SingleToken* b, bool ignoreSize)
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
    return tokenTextPoolEquals(a->text, b->text);
}

void ZydisTokenizer::addToken(TokenType type, QString text, const TokenValue & value)
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
    _inst.tokens.push_back(SingleToken(isNop ? TokenType::MnemonicNop : type, text, value));
}

void ZydisTokenizer::addToken(TokenType type, const QString & text)
{
    addToken(type, text, TokenValue());
}

void ZydisTokenizer::addMemoryOperator(char operatorText)
{
    if(_bMemorySpaces)
        addToken(TokenType::MemoryOperatorSpace, " ");
    QString text;
    text += operatorText;
    addToken(TokenType::MemoryOperator, text);
    if(_bMemorySpaces)
        addToken(TokenType::MemoryOperatorSpace, " ");
}

QString ZydisTokenizer::printValue(const TokenValue & value, bool expandModule, int maxModuleLength) const
{
    QString labelText;
    char label_[MAX_LABEL_SIZE] = "";
    char module_[MAX_MODULE_SIZE] = "";
    QString moduleText;
    duint addr = value.value;
    bool bHasLabel = DbgGetLabelAt(addr, SEG_DEFAULT, label_);
    labelText = QString(label_);
    bool bHasModule;
    if(_bNoCurrentModuleText)
    {
        duint size, base;
        base = DbgMemFindBaseAddr(this->GetZydis().Address(), &size);
        if(addr >= base && addr < base + size)
            bHasModule = false;
        else
            bHasModule = (expandModule && DbgGetModuleAt(addr, module_) && !QString(labelText).startsWith("JMP.&"));
    }
    else
        bHasModule = (expandModule && DbgGetModuleAt(addr, module_) && !QString(labelText).startsWith("JMP.&"));
    moduleText = QString(module_);
    if(maxModuleLength != -1)
        moduleText.truncate(maxModuleLength);
    if(moduleText.length())
        moduleText += ".";
    QString addrText = ToHexString(addr);
    QString finalText;
    if(bHasLabel && bHasModule) //<module.label>
        finalText = QString("<%1%2>").arg(moduleText).arg(labelText);
    else if(bHasModule) //module.addr
        finalText = QString("%1%2").arg(moduleText).arg(addrText);
    else if(bHasLabel) //<label>
        finalText = QString("<%1>").arg(labelText);
    else if(_b0xPrefixValues)
        finalText = QString("0x") + addrText;
    else
        finalText = addrText;
    return finalText;
}

bool ZydisTokenizer::tokenizePrefix()
{
    //TODO: what happens with multiple prefixes?
    bool hasPrefix = true;
    QStringList prefixText;

    auto attr = _cp.GetInstr()->attributes;

    if(attr & ZYDIS_ATTRIB_HAS_LOCK)
        prefixText += "lock";
    else if(attr & ZYDIS_ATTRIB_HAS_REP)
        prefixText += "rep";
    else if(attr & ZYDIS_ATTRIB_HAS_REPE)
        prefixText += "repe";
    else if(attr & ZYDIS_ATTRIB_HAS_REPNE)
        prefixText += "repne";
    else if(attr & ZYDIS_ATTRIB_HAS_BOUND)
        prefixText += "bnd";
    else if(attr & ZYDIS_ATTRIB_HAS_XACQUIRE)
        prefixText += "xacquire";
    else if(attr & ZYDIS_ATTRIB_HAS_XRELEASE)
        prefixText += "xrelease";
    else
        hasPrefix = false;

    if(hasPrefix)
    {
        addToken(TokenType::Prefix, prefixText.join(' '));
        addToken(TokenType::Space, " ");
    }

    return true;
}

bool ZydisTokenizer::tokenizeMnemonic()
{
    QString mnemonic = QString(_cp.Mnemonic().c_str());
    _mnemonicType = TokenType::MnemonicNormal;

    if(_cp.IsBranchType(Zydis::BTFar))
        mnemonic += " far";

    if(isNop)
        _mnemonicType = TokenType::MnemonicNop;
    else if(_cp.IsInt3())
        _mnemonicType = TokenType::MnemonicInt3;
    else if(_cp.IsUnusual())
        _mnemonicType = TokenType::MnemonicUnusual;
    else if(_cp.IsBranchType(Zydis::BTCallSem))
        _mnemonicType = TokenType::MnemonicCall;
    else if(_cp.IsBranchType(Zydis::BTCondJmpSem))
        _mnemonicType = TokenType::MnemonicCondJump;
    else if(_cp.IsBranchType(Zydis::BTUncondJmpSem))
        _mnemonicType = TokenType::MnemonicUncondJump;
    else if(_cp.IsBranchType(Zydis::BTRetSem))
        _mnemonicType = TokenType::MnemonicRet;
    else if(_cp.IsPushPop())
        _mnemonicType = TokenType::MnemonicPushPop;

    return tokenizeMnemonic(_mnemonicType, mnemonic);
}

bool ZydisTokenizer::tokenizeMnemonic(TokenType type, const QString & mnemonic)
{
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

bool ZydisTokenizer::tokenizeOperand(const ZydisDecodedOperand & op)
{
    switch(op.type)
    {
    case ZYDIS_OPERAND_TYPE_REGISTER:
        return tokenizeRegOperand(op);
    case ZYDIS_OPERAND_TYPE_IMMEDIATE:
        return tokenizeImmOperand(op);
    case ZYDIS_OPERAND_TYPE_MEMORY:
        return tokenizeMemOperand(op);
    case ZYDIS_OPERAND_TYPE_POINTER:
        return tokenizePtrOperand(op);
    default:
        return tokenizeInvalidOperand(op);
    }
}

bool ZydisTokenizer::tokenizeRegOperand(const ZydisDecodedOperand & op)
{
    auto registerType = TokenType::GeneralRegister;
    auto reg = op.reg;
    auto regClass = ZydisRegisterGetClass(reg.value);

    switch(regClass)
    {
    case ZYDIS_REGCLASS_X87:
        registerType = TokenType::FpuRegister;
        break;
    case ZYDIS_REGCLASS_MMX:
        registerType = TokenType::MmxRegister;
        break;
    case ZYDIS_REGCLASS_XMM:
        registerType = TokenType::XmmRegister;
        break;
    case ZYDIS_REGCLASS_YMM:
        registerType = TokenType::YmmRegister;
        break;
    case ZYDIS_REGCLASS_ZMM:
        registerType = TokenType::ZmmRegister;
        break;
    }

    if(reg.value == ArchValue(ZYDIS_REGISTER_FS, ZYDIS_REGISTER_GS))
        registerType = TokenType::MnemonicUnusual;

    addToken(registerType, _cp.RegName(reg.value));
    return true;
}

bool ZydisTokenizer::tokenizeImmOperand(const ZydisDecodedOperand & op)
{
    duint value;
    TokenType valueType;
    if(_cp.IsBranchType(Zydis::BTJmp | Zydis::BTCall | Zydis::BTLoop | Zydis::BTXbegin))
    {
        valueType = TokenType::Address;
        value = op.imm.value.u;
    }
    else
    {
        auto opsize = _cp.GetInstr()->operandWidth;
        valueType = TokenType::Value;
        value = duint(op.imm.value.u) & (duint(-1) >> (sizeof(duint) * 8 - opsize));

    }
    auto tokenValue = TokenValue(op.size / 8, value);
    addToken(valueType, printValue(tokenValue, true, _maxModuleLength), tokenValue);
    return true;
}

bool ZydisTokenizer::tokenizeMemOperand(const ZydisDecodedOperand & op)
{
    auto opsize = op.size / 8;

    //memory size
    if(!_bHidePointerSizes)
    {
        const char* sizeText = _cp.MemSizeName(opsize);
        if(sizeText)
        {
            addToken(TokenType::MemorySize, QString(sizeText) + " ptr");
            addToken(TokenType::Space, " ");
        }
    }

    const auto & mem = op.mem;

    //memory segment
    bool bUnusualSegment = (mem.segment == ZYDIS_REGISTER_FS || mem.segment == ZYDIS_REGISTER_GS);
    if(!_bHideNormalSegments || bUnusualSegment)
    {
        auto segmentType = mem.segment == ArchValue(ZYDIS_REGISTER_FS, ZYDIS_REGISTER_GS)
                           ? TokenType::MnemonicUnusual : TokenType::MemorySegment;
        addToken(segmentType, _cp.RegName(mem.segment));
        addToken(TokenType::Uncategorized, ":");
    }

    //memory opening bracket
    auto bracketsType = TokenType::MemoryBrackets;
    switch(mem.base)
    {
    case ZYDIS_REGISTER_ESP:
    case ZYDIS_REGISTER_RSP:
    case ZYDIS_REGISTER_EBP:
    case ZYDIS_REGISTER_RBP:
        bracketsType = TokenType::MemoryStackBrackets;
    default:
        break;
    }
    addToken(bracketsType, "[");

    //stuff inside the brackets
    if(mem.base == ZYDIS_REGISTER_RIP) //rip-relative (#replacement)
    {
        duint addr = _cp.Address() + duint(mem.disp.value) + _cp.Size();
        TokenValue value = TokenValue(opsize, addr);
        auto displacementType = DbgMemIsValidReadPtr(addr) ? TokenType::Address : TokenType::Value;
        addToken(displacementType, printValue(value, false, _maxModuleLength), value);
    }
    else //#base + #index * #scale + #displacement
    {
        bool prependPlus = false;
        if(mem.base != ZYDIS_REGISTER_NONE) //base register
        {
            addToken(TokenType::MemoryBaseRegister, _cp.RegName(mem.base));
            prependPlus = true;
        }
        if(mem.index != ZYDIS_REGISTER_NONE) //index register
        {
            if(prependPlus)
                addMemoryOperator('+');
            addToken(TokenType::MemoryIndexRegister, _cp.RegName(mem.index));
            if(mem.scale > 1)
            {
                addMemoryOperator('*');
                addToken(TokenType::MemoryScale, QString().sprintf("%d", mem.scale));
            }
            prependPlus = true;
        }
        if(mem.disp.value)
        {
            char operatorText = '+';
            TokenValue value(opsize, duint(mem.disp.value));
            auto displacementType = DbgMemIsValidReadPtr(duint(mem.disp.value)) ? TokenType::Address : TokenType::Value;
            QString valueText;
            if(mem.disp.value < 0 && prependPlus)
            {
                operatorText = '-';
                valueText = printValue(TokenValue(opsize, duint(mem.disp.value * -1)), false, _maxModuleLength);
            }
            else
                valueText = printValue(value, false, _maxModuleLength);
            if(prependPlus)
                addMemoryOperator(operatorText);
            addToken(displacementType, valueText, value);
        }
        else if(!prependPlus)
            addToken(TokenType::Value, "0");
    }

    //closing bracket
    addToken(bracketsType, "]");
    return true;
}

bool ZydisTokenizer::tokenizePtrOperand(const ZydisDecodedOperand & op)
{
    auto segValue = TokenValue(2, op.ptr.segment);
    addToken(TokenType::MemorySegment, printValue(segValue, true, _maxModuleLength), segValue);

    addToken(TokenType::Uncategorized, ":");

    auto offsetValue = TokenValue(_cp.GetInstr()->operandWidth / 8, op.ptr.offset);
    addToken(TokenType::Address, printValue(offsetValue, true, _maxModuleLength), offsetValue);

    return true;
}

bool ZydisTokenizer::tokenizeInvalidOperand(const ZydisDecodedOperand & op)
{
    addToken(TokenType::MnemonicUnusual, "???");
    return true;
}
