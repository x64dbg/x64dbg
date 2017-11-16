#include "cs_capstone_gui.h"
#include "Configuration.h"
#include "StringUtil.h"
#include "CachedFontMetrics.h"

CsCapstoneTokenizer::CsCapstoneTokenizer(int maxModuleLength)
    : _maxModuleLength(maxModuleLength),
      _success(false),
      isNop(false),
      _mnemonicType(CapstoneTokenizer::TokenType::Uncategorized)
{
    SetConfig(false, false, false, false, false, false, false, false, false);
}

static CapstoneTokenizer::TokenColor colorNamesMap[CapstoneTokenizer::TokenType::Last];
QHash<QString, int> CsCapstoneTokenizer::stringPoolMap;
int CsCapstoneTokenizer::poolId = 0;

void CsCapstoneTokenizer::addColorName(CapstoneTokenizer::TokenType type, QString color, QString backgroundColor)
{
    colorNamesMap[int(type)] = CapstoneTokenizer::TokenColor(color, backgroundColor);
}

void CsCapstoneTokenizer::addStringsToPool(const QString & strings)
{
    QStringList stringList = strings.split(' ', QString::SkipEmptyParts);
    for(const QString & string : stringList)
        stringPoolMap.insert(string, poolId);
    poolId++;
}

void CsCapstoneTokenizer::UpdateColors()
{
    //filling
    addColorName(CapstoneTokenizer::TokenType::Comma, "InstructionCommaColor", "InstructionCommaBackgroundColor");
    addColorName(CapstoneTokenizer::TokenType::Space, "", "");
    addColorName(CapstoneTokenizer::TokenType::ArgumentSpace, "", "");
    addColorName(CapstoneTokenizer::TokenType::MemoryOperatorSpace, "", "");
    //general instruction parts
    addColorName(CapstoneTokenizer::TokenType::Prefix, "InstructionPrefixColor", "InstructionPrefixBackgroundColor");
    addColorName(CapstoneTokenizer::TokenType::Uncategorized, "InstructionUncategorizedColor", "InstructionUncategorizedBackgroundColor");
    addColorName(CapstoneTokenizer::TokenType::Address, "InstructionAddressColor", "InstructionAddressBackgroundColor"); //jump/call destinations
    addColorName(CapstoneTokenizer::TokenType::Value, "InstructionValueColor", "InstructionValueBackgroundColor");
    //mnemonics
    addColorName(CapstoneTokenizer::TokenType::MnemonicNormal, "InstructionMnemonicColor", "InstructionMnemonicBackgroundColor");
    addColorName(CapstoneTokenizer::TokenType::MnemonicPushPop, "InstructionPushPopColor", "InstructionPushPopBackgroundColor");
    addColorName(CapstoneTokenizer::TokenType::MnemonicCall, "InstructionCallColor", "InstructionCallBackgroundColor");
    addColorName(CapstoneTokenizer::TokenType::MnemonicRet, "InstructionRetColor", "InstructionRetBackgroundColor");
    addColorName(CapstoneTokenizer::TokenType::MnemonicCondJump, "InstructionConditionalJumpColor", "InstructionConditionalJumpBackgroundColor");
    addColorName(CapstoneTokenizer::TokenType::MnemonicUncondJump, "InstructionUnconditionalJumpColor", "InstructionUnconditionalJumpBackgroundColor");
    addColorName(CapstoneTokenizer::TokenType::MnemonicNop, "InstructionNopColor", "InstructionNopBackgroundColor");
    addColorName(CapstoneTokenizer::TokenType::MnemonicFar, "InstructionFarColor", "InstructionFarBackgroundColor");
    addColorName(CapstoneTokenizer::TokenType::MnemonicInt3, "InstructionInt3Color", "InstructionInt3BackgroundColor");
    addColorName(CapstoneTokenizer::TokenType::MnemonicUnusual, "InstructionUnusualColor", "InstructionUnusualBackgroundColor");
    //memory
    addColorName(CapstoneTokenizer::TokenType::MemorySize, "InstructionMemorySizeColor", "InstructionMemorySizeBackgroundColor");
    addColorName(CapstoneTokenizer::TokenType::MemorySegment, "InstructionMemorySegmentColor", "InstructionMemorySegmentBackgroundColor");
    addColorName(CapstoneTokenizer::TokenType::MemoryBrackets, "InstructionMemoryBracketsColor", "InstructionMemoryBracketsBackgroundColor");
    addColorName(CapstoneTokenizer::TokenType::MemoryStackBrackets, "InstructionMemoryStackBracketsColor", "InstructionMemoryStackBracketsBackgroundColor");
    addColorName(CapstoneTokenizer::TokenType::MemoryBaseRegister, "InstructionMemoryBaseRegisterColor", "InstructionMemoryBaseRegisterBackgroundColor");
    addColorName(CapstoneTokenizer::TokenType::MemoryIndexRegister, "InstructionMemoryIndexRegisterColor", "InstructionMemoryIndexRegisterBackgroundColor");
    addColorName(CapstoneTokenizer::TokenType::MemoryScale, "InstructionMemoryScaleColor", "InstructionMemoryScaleBackgroundColor");
    addColorName(CapstoneTokenizer::TokenType::MemoryOperator, "InstructionMemoryOperatorColor", "InstructionMemoryOperatorBackgroundColor");
    //registers
    addColorName(CapstoneTokenizer::TokenType::GeneralRegister, "InstructionGeneralRegisterColor", "InstructionGeneralRegisterBackgroundColor");
    addColorName(CapstoneTokenizer::TokenType::FpuRegister, "InstructionFpuRegisterColor", "InstructionFpuRegisterBackgroundColor");
    addColorName(CapstoneTokenizer::TokenType::MmxRegister, "InstructionMmxRegisterColor", "InstructionMmxRegisterBackgroundColor");
    addColorName(CapstoneTokenizer::TokenType::XmmRegister, "InstructionXmmRegisterColor", "InstructionXmmRegisterBackgroundColor");
    addColorName(CapstoneTokenizer::TokenType::YmmRegister, "InstructionYmmRegisterColor", "InstructionYmmRegisterBackgroundColor");
    addColorName(CapstoneTokenizer::TokenType::ZmmRegister, "InstructionZmmRegisterColor", "InstructionZmmRegisterBackgroundColor");
}

void CsCapstoneTokenizer::UpdateStringPool()
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

bool CsCapstoneTokenizer::Tokenize(duint addr, const unsigned char* data, int datasize, CapstoneTokenizer::InstructionToken & instruction)
{
    _inst = CapstoneTokenizer::InstructionToken();

    _success = _cp.DisassembleSafe(addr, data, datasize);
    if(_success)
    {
        isNop = _cp.IsNop();
        if(!tokenizeMnemonic())
            return false;

        for(int i = 0; i < _cp.OpCount(); i++)
        {
            if(i)
            {
                addToken(CapstoneTokenizer::TokenType::Comma, ",");
                if(_bArgumentSpaces)
                    addToken(CapstoneTokenizer::TokenType::ArgumentSpace, " ");
            }
            if(!tokenizeOperand(_cp[i]))
                return false;
        }
    }
    else
    {
        isNop = false;
        addToken(CapstoneTokenizer::TokenType::MnemonicUnusual, "???");
    }

    if(_bNoHighlightOperands)
    {
        while(_inst.tokens.size() && _inst.tokens[_inst.tokens.size() - 1].type == CapstoneTokenizer::TokenType::Space)
            _inst.tokens.pop_back();
        for(CapstoneTokenizer::SingleToken & token : _inst.tokens)
            token.type = _mnemonicType;
    }

    instruction = _inst;

    return true;
}

bool CsCapstoneTokenizer::TokenizeData(const QString & datatype, const QString & data, CapstoneTokenizer::InstructionToken & instruction)
{
    _inst = CapstoneTokenizer::InstructionToken();
    isNop = false;

    if(!tokenizeMnemonic(CapstoneTokenizer::TokenType::MnemonicNormal, datatype))
        return false;

    addToken(CapstoneTokenizer::TokenType::Value, data);

    instruction = _inst;

    return true;
}

void CsCapstoneTokenizer::UpdateConfig()
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

void CsCapstoneTokenizer::SetConfig(bool bUppercase, bool bTabbedMnemonic, bool bArgumentSpaces, bool bHidePointerSizes, bool bHideNormalSegments, bool bMemorySpaces, bool bNoHighlightOperands, bool bNoCurrentModuleText, bool b0xPrefixValues)
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

int CsCapstoneTokenizer::Size() const
{
    return _success ? _cp.Size() : 1;
}

const Capstone & CsCapstoneTokenizer::GetCapstone() const
{
    return _cp;
}

void CsCapstoneTokenizer::TokenToRichText(const CapstoneTokenizer::InstructionToken & instr, RichTextPainter::List & richTextList, const CapstoneTokenizer::SingleToken* highlightToken)
{
    QColor highlightColor = ConfigColor("InstructionHighlightColor");
    for(const auto & token : instr.tokens)
    {
        RichTextPainter::CustomRichText_t richText;
        richText.highlight = TokenEquals(&token, highlightToken);
        richText.highlightColor = highlightColor;
        richText.flags = RichTextPainter::FlagNone;
        richText.text = token.text;
        if(token.type < CapstoneTokenizer::TokenType::Last)
        {
            const auto & tokenColor = colorNamesMap[int(token.type)];
            richText.flags = tokenColor.flags;
            richText.textColor = tokenColor.color;
            richText.textBackground = tokenColor.backgroundColor;
        }
        richTextList.push_back(richText);
    }
}

bool CsCapstoneTokenizer::TokenFromX(const CapstoneTokenizer::InstructionToken & instr, CapstoneTokenizer::SingleToken & token, int x, CachedFontMetrics* fontMetrics)
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

bool CsCapstoneTokenizer::IsHighlightableToken(const CapstoneTokenizer::SingleToken & token)
{
    switch(token.type)
    {
    case CapstoneTokenizer::TokenType::Comma:
    case CapstoneTokenizer::TokenType::Space:
    case CapstoneTokenizer::TokenType::ArgumentSpace:
    case CapstoneTokenizer::TokenType::Uncategorized:
    case CapstoneTokenizer::TokenType::MemoryOperatorSpace:
    case CapstoneTokenizer::TokenType::MemoryBrackets:
    case CapstoneTokenizer::TokenType::MemoryStackBrackets:
    case CapstoneTokenizer::TokenType::MemoryOperator:
        return false;
        break;
    }
    return true;
}

bool CsCapstoneTokenizer::tokenTextPoolEquals(const QString & a, const QString & b)
{
    if(a.compare(b, Qt::CaseInsensitive) == 0)
        return true;
    auto found1 = stringPoolMap.find(a.toLower());
    auto found2 = stringPoolMap.find(b.toLower());
    if(found1 == stringPoolMap.end() || found2 == stringPoolMap.end())
        return false;
    return found1.value() == found2.value();
}

bool CsCapstoneTokenizer::TokenEquals(const CapstoneTokenizer::SingleToken* a, const CapstoneTokenizer::SingleToken* b, bool ignoreSize)
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

void CsCapstoneTokenizer::addToken(CapstoneTokenizer::TokenType type, QString text, const CapstoneTokenizer::TokenValue & value)
{
    switch(type)
    {
    case CapstoneTokenizer::TokenType::Space:
    case CapstoneTokenizer::TokenType::ArgumentSpace:
    case CapstoneTokenizer::TokenType::MemoryOperatorSpace:
        break;
    default:
        text = text.trimmed();
        break;
    }
    if(_bUppercase && !value.size)
        text = text.toUpper();
    _inst.tokens.push_back(CapstoneTokenizer::SingleToken(isNop ? CapstoneTokenizer::TokenType::MnemonicNop : type, text, value));
}

void CsCapstoneTokenizer::addToken(CapstoneTokenizer::TokenType type, const QString & text)
{
    addToken(type, text, CapstoneTokenizer::TokenValue());
}

void CsCapstoneTokenizer::addMemoryOperator(char operatorText)
{
    if(_bMemorySpaces)
        addToken(CapstoneTokenizer::TokenType::MemoryOperatorSpace, " ");
    QString text;
    text += operatorText;
    addToken(CapstoneTokenizer::TokenType::MemoryOperator, text);
    if(_bMemorySpaces)
        addToken(CapstoneTokenizer::TokenType::MemoryOperatorSpace, " ");
}

QString CsCapstoneTokenizer::printValue(const CapstoneTokenizer::TokenValue & value, bool expandModule, int maxModuleLength) const
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
        base = DbgMemFindBaseAddr(this->GetCapstone().Address(), &size);
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

bool CsCapstoneTokenizer::tokenizePrefix()
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
        addToken(CapstoneTokenizer::TokenType::Prefix, prefixText);
        addToken(CapstoneTokenizer::TokenType::Space, " ");
    }

    return true;
}

bool CsCapstoneTokenizer::tokenizeMnemonic()
{
    QString mnemonic = QString(_cp.Mnemonic().c_str());
    _mnemonicType = CapstoneTokenizer::TokenType::MnemonicNormal;
    auto id = _cp.GetId();
    if(isNop)
        _mnemonicType = CapstoneTokenizer::TokenType::MnemonicNop;
    else if(_cp.InGroup(CS_GRP_CALL))
        _mnemonicType = CapstoneTokenizer::TokenType::MnemonicCall;
    else if(_cp.InGroup(CS_GRP_JUMP) || _cp.IsLoop())
    {
        switch(id)
        {
        case X86_INS_JMP:
        case X86_INS_LJMP:
            _mnemonicType = CapstoneTokenizer::TokenType::MnemonicUncondJump;
            break;
        default:
            _mnemonicType = CapstoneTokenizer::TokenType::MnemonicCondJump;
            break;
        }
    }
    else if(_cp.IsInt3())
        _mnemonicType = CapstoneTokenizer::TokenType::MnemonicInt3;
    else if(_cp.IsUnusual())
        _mnemonicType = CapstoneTokenizer::TokenType::MnemonicUnusual;
    else if(_cp.InGroup(CS_GRP_RET))
        _mnemonicType = CapstoneTokenizer::TokenType::MnemonicRet;
    else
    {
        switch(id)
        {
        case X86_INS_PUSH:
        case X86_INS_PUSHF:
        case X86_INS_PUSHFD:
        case X86_INS_PUSHFQ:
        case X86_INS_PUSHAL:
        case X86_INS_PUSHAW:
        case X86_INS_POP:
        case X86_INS_POPF:
        case X86_INS_POPFD:
        case X86_INS_POPFQ:
        case X86_INS_POPAL:
        case X86_INS_POPAW:
            _mnemonicType = CapstoneTokenizer::TokenType::MnemonicPushPop;
            break;
        default:
            break;
        }
    }

    tokenizeMnemonic(_mnemonicType, mnemonic);

    return true;
}

bool CsCapstoneTokenizer::tokenizeMnemonic(CapstoneTokenizer::TokenType type, const QString & mnemonic)
{
    addToken(type, mnemonic);
    if(_bTabbedMnemonic)
    {
        int spaceCount = 7 - mnemonic.length();
        if(spaceCount > 0)
        {
            for(int i = 0; i < spaceCount; i++)
                addToken(CapstoneTokenizer::TokenType::Space, " ");
        }
    }
    addToken(CapstoneTokenizer::TokenType::Space, " ");
    return true;
}

bool CsCapstoneTokenizer::tokenizeOperand(const cs_x86_op & op)
{
    switch(op.type)
    {
    case X86_OP_REG:
        return tokenizeRegOperand(op);
    case X86_OP_IMM:
        return tokenizeImmOperand(op);
    case X86_OP_MEM:
        return tokenizeMemOperand(op);
    case X86_OP_INVALID:
        return tokenizeInvalidOperand(op);
    default:
        return false;
    }
}

bool CsCapstoneTokenizer::tokenizeRegOperand(const cs_x86_op & op)
{
    auto registerType = CapstoneTokenizer::TokenType::GeneralRegister;
    auto reg = op.reg;
    if(reg >= X86_REG_FP0 && reg <= X86_REG_FP7)
        registerType = CapstoneTokenizer::TokenType::FpuRegister;
    else if(reg >= X86_REG_ST0 && reg <= X86_REG_ST7)
        registerType = CapstoneTokenizer::TokenType::FpuRegister;
    else if(reg >= X86_REG_MM0 && reg <= X86_REG_MM7)
        registerType = CapstoneTokenizer::TokenType::MmxRegister;
    else if(reg >= X86_REG_XMM0 && reg <= X86_REG_XMM31)
        registerType = CapstoneTokenizer::TokenType::XmmRegister;
    else if(reg >= X86_REG_YMM0 && reg <= X86_REG_YMM31)
        registerType = CapstoneTokenizer::TokenType::YmmRegister;
    else if(reg >= X86_REG_ZMM0 && reg <= X86_REG_ZMM31)
        registerType = CapstoneTokenizer::TokenType::ZmmRegister;
    else if(reg == ArchValue(X86_REG_FS, X86_REG_GS))
        registerType = CapstoneTokenizer::TokenType::MnemonicUnusual;
    addToken(registerType, _cp.RegName(reg));
    return true;
}

bool CsCapstoneTokenizer::tokenizeImmOperand(const cs_x86_op & op)
{
    auto value = duint(op.imm) & (duint(-1) >> (op.size ? 8 * (sizeof(duint) - op.size) : 0));
    auto valueType = CapstoneTokenizer::TokenType::Value;
    if(_cp.InGroup(CS_GRP_JUMP) || _cp.InGroup(CS_GRP_CALL) || _cp.IsLoop())
        valueType = CapstoneTokenizer::TokenType::Address;
    auto tokenValue = CapstoneTokenizer::TokenValue(op.size, value);
    addToken(valueType, printValue(tokenValue, true, _maxModuleLength), tokenValue);
    return true;
}

bool CsCapstoneTokenizer::tokenizeMemOperand(const cs_x86_op & op)
{
    //memory size
    if(!_bHidePointerSizes)
    {
        const char* sizeText = _cp.MemSizeName(op.size);
        if(!sizeText)
            return false;
        addToken(CapstoneTokenizer::TokenType::MemorySize, QString(sizeText) + " ptr");
        addToken(CapstoneTokenizer::TokenType::Space, " ");
    }

    const auto & mem = op.mem;

    //memory segment
    bool bUnusualSegment = (mem.segment == X86_REG_FS || mem.segment == X86_REG_GS);
    if(!_bHideNormalSegments || bUnusualSegment)
    {
        const char* segmentText = _cp.RegName(mem.segment);
        if(mem.segment == X86_REG_INVALID) //segment not set
        {
            switch(mem.base)
            {
            case X86_REG_RSP:
            case X86_REG_RBP:
            case X86_REG_ESP:
            case X86_REG_EBP:
            case X86_REG_SP:
            case X86_REG_BP:
                segmentText = "ss";
                break;
            default:
                segmentText = "ds";
                break;
            }
        }
        auto segmentType = op.reg == ArchValue(X86_REG_FS, X86_REG_GS)
                           ? CapstoneTokenizer::TokenType::MnemonicUnusual : CapstoneTokenizer::TokenType::MemorySegment;
        addToken(segmentType, segmentText);
        addToken(CapstoneTokenizer::TokenType::Uncategorized, ":");
    }

    //memory opening bracket
    auto bracketsType = CapstoneTokenizer::TokenType::MemoryBrackets;
    switch(mem.base)
    {
    case X86_REG_ESP:
    case X86_REG_RSP:
    case X86_REG_EBP:
    case X86_REG_RBP:
        bracketsType = CapstoneTokenizer::TokenType::MemoryStackBrackets;
    default:
        break;
    }
    addToken(bracketsType, "[");

    //stuff inside the brackets
    if(mem.base == X86_REG_RIP) //rip-relative (#replacement)
    {
        duint addr = _cp.Address() + duint(mem.disp) + _cp.Size();
        CapstoneTokenizer::TokenValue value = CapstoneTokenizer::TokenValue(op.size, addr);
        auto displacementType = DbgMemIsValidReadPtr(addr) ? CapstoneTokenizer::TokenType::Address : CapstoneTokenizer::TokenType::Value;
        addToken(displacementType, printValue(value, false, _maxModuleLength), value);
    }
    else //#base + #index * #scale + #displacement
    {
        bool prependPlus = false;
        if(mem.base != X86_REG_INVALID) //base register
        {
            addToken(CapstoneTokenizer::TokenType::MemoryBaseRegister, _cp.RegName(mem.base));
            prependPlus = true;
        }
        if(mem.index != X86_REG_INVALID) //index register
        {
            if(prependPlus)
                addMemoryOperator('+');
            addToken(CapstoneTokenizer::TokenType::MemoryIndexRegister, _cp.RegName(mem.index));
            if(mem.scale > 1)
            {
                addMemoryOperator('*');
                addToken(CapstoneTokenizer::TokenType::MemoryScale, QString().sprintf("%d", mem.scale));
            }
            prependPlus = true;
        }
        if(mem.disp)
        {
            char operatorText = '+';
            CapstoneTokenizer::TokenValue value(op.size, duint(mem.disp));
            auto displacementType = DbgMemIsValidReadPtr(duint(mem.disp)) ? CapstoneTokenizer::TokenType::Address : CapstoneTokenizer::TokenType::Value;
            QString valueText;
            if(mem.disp < 0 && prependPlus)
            {
                operatorText = '-';
                valueText = printValue(CapstoneTokenizer::TokenValue(op.size, duint(mem.disp * -1)), false, _maxModuleLength);
            }
            else
                valueText = printValue(value, false, _maxModuleLength);
            if(prependPlus)
                addMemoryOperator(operatorText);
            addToken(displacementType, valueText, value);
        }
        else if(!prependPlus)
            addToken(CapstoneTokenizer::TokenType::Value, "0");
    }

    //closing bracket
    addToken(bracketsType, "]");
    return true;
}

bool CsCapstoneTokenizer::tokenizeInvalidOperand(const cs_x86_op & op)
{
    addToken(CapstoneTokenizer::TokenType::MnemonicUnusual, "???");
    return true;
}
