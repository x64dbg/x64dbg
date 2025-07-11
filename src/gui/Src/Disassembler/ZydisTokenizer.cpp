#include "ZydisTokenizer.h"
#include "Configuration.h"
#include "StringUtil.h"
#include "CachedFontMetrics.h"
#include "Bridge.h"

ZydisTokenizer::ZydisTokenizer(int maxModuleLength, Architecture* architecture)
    : mMaxModuleLength(maxModuleLength),
      mZydis(architecture->disasm64()),
      mArchitecture(architecture)
{
}

static ZydisTokenizer::TokenColor colorNamesMap[size_t(ZydisTokenizer::TokenType::Last)];
QHash<QString, int> ZydisTokenizer::gStringPool;
int ZydisTokenizer::gPoolId = 0;

void ZydisTokenizer::addColorName(TokenType type, QString color, QString backgroundColor)
{
    colorNamesMap[int(type)] = TokenColor(color, backgroundColor);
}

ZydisTokenizer::TokenColor ZydisTokenizer::getTokenColor(TokenType type)
{
    return colorNamesMap[(size_t)type];
}

void ZydisTokenizer::addStringsToPool(const QString & strings)
{
    QStringList stringList = strings.split(' ', Qt::SkipEmptyParts);
    for(const QString & string : stringList)
        gStringPool.insert(string, gPoolId);
    gPoolId++;
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
    gPoolId = 0;
    gStringPool.clear();
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
    addStringsToPool("xmm0 ymm0 zmm0");
    addStringsToPool("xmm1 ymm1 zmm1");
    addStringsToPool("xmm2 ymm2 zmm2");
    addStringsToPool("xmm3 ymm3 zmm3");
    addStringsToPool("xmm4 ymm4 zmm4");
    addStringsToPool("xmm5 ymm5 zmm5");
    addStringsToPool("xmm6 ymm6 zmm6");
    addStringsToPool("xmm7 ymm7 zmm7");
    addStringsToPool("xmm8 ymm8 zmm8");
    addStringsToPool("xmm9 ymm9 zmm9");
    addStringsToPool("xmm10 ymm10 zmm10");
    addStringsToPool("xmm11 ymm11 zmm11");
    addStringsToPool("xmm12 ymm12 zmm12");
    addStringsToPool("xmm13 ymm13 zmm13");
    addStringsToPool("xmm14 ymm14 zmm14");
    addStringsToPool("xmm15 ymm15 zmm15");
    addStringsToPool("xmm16 ymm16 zmm16");
    addStringsToPool("xmm17 ymm17 zmm17");
    addStringsToPool("xmm18 ymm18 zmm18");
    addStringsToPool("xmm19 ymm19 zmm19");
    addStringsToPool("xmm20 ymm20 zmm20");
    addStringsToPool("xmm21 ymm21 zmm21");
    addStringsToPool("xmm22 ymm22 zmm22");
    addStringsToPool("xmm23 ymm23 zmm23");
    addStringsToPool("xmm24 ymm24 zmm24");
    addStringsToPool("xmm25 ymm25 zmm25");
    addStringsToPool("xmm26 ymm26 zmm26");
    addStringsToPool("xmm27 ymm27 zmm27");
    addStringsToPool("xmm28 ymm28 zmm28");
    addStringsToPool("xmm29 ymm29 zmm29");
    addStringsToPool("xmm30 ymm30 zmm30");
    addStringsToPool("xmm31 ymm31 zmm31");
}

bool ZydisTokenizer::Tokenize(duint addr, const unsigned char* data, int datasize, InstructionToken & instruction)
{
    mInst = InstructionToken();

    mSuccess = mZydis.DisassembleSafe(addr, data, datasize);
    if(mSuccess)
    {
        if(!tokenizePrefix())
            return false;

        mIsNop = mZydis.IsNop();
        if(!tokenizeMnemonic())
            return false;

        for(int i = 0; i < mZydis.OpCount(); i++)
        {
            if(i == 1 && mZydis[0].size >= 128 && mZydis[1].type == ZYDIS_OPERAND_TYPE_REGISTER
                    && ZydisRegisterGetClass(mZydis[1].reg.value) == ZYDIS_REGCLASS_MASK)
            {
                if(mArgumentSpaces)
                    addToken(TokenType::ArgumentSpace, " ");
                addToken(TokenType::Comma, "{");
                if(!tokenizeOperand(mZydis[i]))
                    return false;
                addToken(TokenType::Comma, "}");
            }
            else if(i)
            {
                addToken(TokenType::Comma, ",");
                if(mArgumentSpaces)
                    addToken(TokenType::ArgumentSpace, " ");
                if(!tokenizeOperand(mZydis[i]))
                    return false;
            }
            else
            {
                if(!tokenizeOperand(mZydis[i]))
                    return false;
            }
        }
    }
    else
    {
        mIsNop = false;
        addToken(TokenType::MnemonicUnusual, "???");
    }

    if(mNoHighlightOperands)
    {
        while(mInst.tokens.size() && mInst.tokens[mInst.tokens.size() - 1].type == TokenType::Space)
            mInst.tokens.pop_back();
        for(SingleToken & token : mInst.tokens)
            token.type = mMnemonicType;
    }

    instruction = mInst;

    return true;
}

bool ZydisTokenizer::TokenizeData(const QString & datatype, const QString & data, InstructionToken & instruction)
{
    mInst = InstructionToken();
    mIsNop = false;

    if(!tokenizeMnemonic(TokenType::MnemonicNormal, datatype))
        return false;

    addToken(TokenType::Value, data);

    instruction = mInst;

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
    mUppercase = ConfigBool("Disassembler", "Uppercase");
    mTabbedMnemonic = ConfigBool("Disassembler", "TabbedMnemonic");
    mArgumentSpaces = ConfigBool("Disassembler", "ArgumentSpaces");
    mHidePointerSizes = ConfigBool("Disassembler", "HidePointerSizes");
    mHideNormalSegments = ConfigBool("Disassembler", "HideNormalSegments");
    mMemorySpaces = ConfigBool("Disassembler", "MemorySpaces");
    mNoHighlightOperands = ConfigBool("Disassembler", "NoHighlightOperands");
    mNoCurrentModuleText = ConfigBool("Disassembler", "NoCurrentModuleText");
    mValueNotation = (DisasmValueNotationType)ConfigUint("Disassembler", "0xPrefixValues");
    mMaxModuleLength = (int)ConfigUint("Disassembler", "MaxModuleSize");
    UpdateStringPool();
}

void ZydisTokenizer::UpdateArchitecture()
{
    mZydis.Reset(mArchitecture->disasm64());
}

void ZydisTokenizer::SetConfig(bool bUppercase, bool bTabbedMnemonic, bool bArgumentSpaces, bool bHidePointerSizes, bool bHideNormalSegments, bool bMemorySpaces, bool bNoHighlightOperands, bool bNoCurrentModuleText, DisasmValueNotationType ValueNotation)
{
    mUppercase = bUppercase;
    mTabbedMnemonic = bTabbedMnemonic;
    mArgumentSpaces = bArgumentSpaces;
    mHidePointerSizes = bHidePointerSizes;
    mHideNormalSegments = bHideNormalSegments;
    mMemorySpaces = bMemorySpaces;
    mNoHighlightOperands = bNoHighlightOperands;
    mNoCurrentModuleText = bNoCurrentModuleText;
    mValueNotation = ValueNotation;
}

int ZydisTokenizer::Size() const
{
    return mSuccess ? mZydis.Size() : 1;
}

const Zydis & ZydisTokenizer::GetZydis() const
{
    return mZydis;
}

void ZydisTokenizer::TokenToRichText(const InstructionToken & instr, RichTextPainter::List & richTextList, const SingleToken* highlightToken)
{
    QColor highlightColor = ConfigColor("InstructionHighlightColor");
    QColor highlightBackgroundColor = ConfigColor("InstructionHighlightBackgroundColor");
    for(const auto & token : instr.tokens)
    {
        RichTextPainter::CustomRichText_t richText;
        richText.flags = RichTextPainter::FlagNone;
        richText.text = token.text;
        richText.underline = false;
        if(token.type < TokenType::Last)
        {
            const auto & tokenColor = colorNamesMap[int(token.type)];
            richText.flags = tokenColor.flags;
            if(TokenEquals(&token, highlightToken))
            {
                richText.textColor = highlightColor;
                richText.textBackground = highlightBackgroundColor;
            }
            else
            {
                richText.textColor = tokenColor.color;
                richText.textBackground = tokenColor.backgroundColor;
            }
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
    default:
        return true;
    }
}

bool ZydisTokenizer::tokenTextPoolEquals(const QString & a, const QString & b)
{
    if(a.compare(b, Qt::CaseInsensitive) == 0)
        return true;
    auto found1 = gStringPool.find(a.toLower());
    auto found2 = gStringPool.find(b.toLower());
    if(found1 == gStringPool.end() || found2 == gStringPool.end())
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


static bool tokenIsSpace(ZydisTokenizer::TokenType type)
{
    return type == ZydisTokenizer::TokenType::Space || type == ZydisTokenizer::TokenType::ArgumentSpace || type == ZydisTokenizer::TokenType::MemoryOperatorSpace;
}

void ZydisTokenizer::addToken(TokenType type, QString text, const TokenValue & value)
{
    bool isItSpaceType = tokenIsSpace(type);
    if(!isItSpaceType)
    {
        text = text.trimmed();
    }

    if(mUppercase && !value.size)
        text = text.toUpper();

    if(isItSpaceType)
    {
        mInst.tokens.push_back(SingleToken(type, text, value));
    }
    else if(type == TokenType::Address && text.startsWith("<&"))
    {
        mInst.tokens.push_back(SingleToken(TokenType::Address, "<", value));
        mInst.tokens.push_back(SingleToken(TokenType::MemoryBaseRegister, "&", value));
        mInst.tokens.push_back(SingleToken(TokenType::Address, text.mid(2)));
    }
    else
    {
        mInst.tokens.push_back(SingleToken(mIsNop ? TokenType::MnemonicNop : type, text, value));
    }
}

void ZydisTokenizer::addToken(TokenType type, const QString & text)
{
    addToken(type, text, TokenValue());
}

void ZydisTokenizer::addMemoryOperator(char operatorText)
{
    if(mMemorySpaces)
        addToken(TokenType::MemoryOperatorSpace, " ");
    QString text;
    text += operatorText;
    addToken(TokenType::MemoryOperator, text);
    if(mMemorySpaces)
        addToken(TokenType::MemoryOperatorSpace, " ");
}

QString ZydisTokenizer::printValue(const TokenValue & value, bool expandModule) const
{
    QString labelText;
    char label[MAX_LABEL_SIZE] = "";
    char module[MAX_MODULE_SIZE] = "";
    QString moduleText;
    duint addr = value.value;
    bool bHasLabel = DbgGetLabelAt(addr, SEG_DEFAULT, label);
    labelText = QString(label);
    bool bHasModule;
    if(mNoCurrentModuleText)
    {
        duint size, base;
        base = DbgMemFindBaseAddr(this->GetZydis().Address(), &size);
        if(addr >= base && addr < base + size)
            bHasModule = false;
        else
            bHasModule = (expandModule && DbgGetModuleAt(addr, module) && !QString(labelText).startsWith("JMP.&"));
    }
    else
        bHasModule = (expandModule && DbgGetModuleAt(addr, module) && !QString(labelText).startsWith("JMP.&"));
    moduleText = QString(module);
    if(mMaxModuleLength != -1)
        moduleText.truncate(mMaxModuleLength);
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
    {
        switch(mValueNotation)
        {
        case DisasmValueNotationC:
            finalText = QString("0x") + addrText;
            break;
        case DisasmValueNotationMASM:
            finalText = QString("0") + addrText + QString("h");
            break;
        default:
            finalText = addrText;
            break;
        }
    }

    return finalText;
}

bool ZydisTokenizer::tokenizePrefix()
{
    auto addPrefix = [this](const QString & prefix)
    {
        addToken(TokenType::Prefix, prefix);
        addToken(TokenType::Space, " ");
    };

    auto attr = mZydis.GetInstr()->info.attributes;
    if(attr & ZYDIS_ATTRIB_HAS_LOCK)
        addPrefix("lock");
    if(attr & ZYDIS_ATTRIB_HAS_REP)
        addPrefix("rep");
    if(attr & ZYDIS_ATTRIB_HAS_REPE)
        addPrefix("repe");
    if(attr & ZYDIS_ATTRIB_HAS_REPNE)
        addPrefix("repne");
    if(attr & ZYDIS_ATTRIB_HAS_BND)
        addPrefix("bnd");
    if(attr & ZYDIS_ATTRIB_HAS_XACQUIRE)
        addPrefix("xacquire");
    if(attr & ZYDIS_ATTRIB_HAS_XRELEASE)
        addPrefix("xrelease");
    if(attr & ZYDIS_ATTRIB_HAS_BRANCH_NOT_TAKEN)
        addPrefix("unlikely");
    if(attr & ZYDIS_ATTRIB_HAS_BRANCH_TAKEN)
        addPrefix("likely");
    if(attr & ZYDIS_ATTRIB_HAS_NOTRACK)
        addPrefix("notrack");

    return true;
}

bool ZydisTokenizer::tokenizeMnemonic()
{
    QString mnemonic = QString(mZydis.Mnemonic().c_str());
    mMnemonicType = TokenType::MnemonicNormal;

    if(mZydis.IsBranchType(Zydis::BTFar))
        mnemonic += " far";

    if(mIsNop)
        mMnemonicType = TokenType::MnemonicNop;
    else if(mZydis.IsInt3())
        mMnemonicType = TokenType::MnemonicInt3;
    else if(mZydis.IsUnusual())
        mMnemonicType = TokenType::MnemonicUnusual;
    else if(mZydis.IsBranchType(Zydis::BTCallSem))
        mMnemonicType = TokenType::MnemonicCall;
    else if(mZydis.IsBranchType(Zydis::BTCondJmpSem))
        mMnemonicType = TokenType::MnemonicCondJump;
    else if(mZydis.IsBranchType(Zydis::BTUncondJmpSem))
        mMnemonicType = TokenType::MnemonicUncondJump;
    else if(mZydis.IsBranchType(Zydis::BTRetSem))
        mMnemonicType = TokenType::MnemonicRet;
    else if(mZydis.IsPushPop())
        mMnemonicType = TokenType::MnemonicPushPop;

    return tokenizeMnemonic(mMnemonicType, mnemonic);
}

bool ZydisTokenizer::tokenizeMnemonic(TokenType type, const QString & mnemonic)
{
    addToken(type, mnemonic);
    if(mTabbedMnemonic)
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
    case ZYDIS_REGCLASS_MASK:
        registerType = TokenType::ZmmRegister;
        break;
    default:
        break;
    }

    if(reg.value == (mArchitecture->disasm64() ? ZYDIS_REGISTER_GS : ZYDIS_REGISTER_FS))
        registerType = TokenType::MnemonicUnusual;

    addToken(registerType, mZydis.RegName(reg.value));
    return true;
}

bool ZydisTokenizer::tokenizeImmOperand(const ZydisDecodedOperand & op)
{
    duint value;
    TokenType valueType;
    if(mZydis.IsBranchType(Zydis::BTJmp | Zydis::BTCall | Zydis::BTLoop | Zydis::BTXbegin))
    {
        valueType = TokenType::Address;
        value = op.imm.value.u;
    }
    else
    {
        auto opsize = mZydis.GetInstr()->info.operand_width;
        valueType = TokenType::Value;
        value = duint(op.imm.value.u) & (duint(-1) >> (sizeof(duint) * 8 - opsize));
    }
    auto tokenValue = TokenValue(op.size / 8, value);
    addToken(valueType, printValue(tokenValue, true), tokenValue);
    return true;
}

bool ZydisTokenizer::tokenizeMemOperand(const ZydisDecodedOperand & op)
{
    auto opsize = op.size / 8;
    const char* sizeText = mZydis.MemSizeName(opsize);

    //memory size
    if(sizeText)
    {
        if(!mHidePointerSizes)
        {
            addToken(TokenType::MemorySize, QString(sizeText) + " ptr");
            addToken(TokenType::Space, " ");
        }
        else
        {
            //ambiguous operand sizes
            for(auto i = 0; i < mZydis.GetInstr()->info.operand_count; i += 1)
            {
                const auto & op_ = mZydis.GetInstr()->operands[i];

                if(op_.id == op.id)
                    continue;

                if(op_.type == ZYDIS_OPERAND_TYPE_REGISTER && op_.size == op.size)
                    continue;

                if(op_.type == ZYDIS_OPERAND_TYPE_MEMORY)
                    continue;

                addToken(TokenType::MemorySize, QString(sizeText) + " ptr");
                addToken(TokenType::Space, " ");
                break;
            }
        }
    }

    const auto & mem = op.mem;

    //memory segment
    bool bUnusualSegment = (mem.segment == ZYDIS_REGISTER_FS || mem.segment == ZYDIS_REGISTER_GS);
    if(!mHideNormalSegments || bUnusualSegment)
    {
        auto segmentType = mem.segment == (mArchitecture->disasm64() ? ZYDIS_REGISTER_GS : ZYDIS_REGISTER_FS)
                           ? TokenType::MnemonicUnusual : TokenType::MemorySegment;
        addToken(segmentType, mZydis.RegName(mem.segment));
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
        duint addr = mZydis.Address() + duint(mem.disp.value) + mZydis.Size();
        TokenValue value = TokenValue(opsize, addr);
        auto displacementType = DbgMemIsValidReadPtr(addr) ? TokenType::Address : TokenType::Value;
        addToken(displacementType, printValue(value, false), value);
    }
    else //#base + #index * #scale + #displacement
    {
        bool prependPlus = false;
        if(mem.base != ZYDIS_REGISTER_NONE) //base register
        {
            addToken(TokenType::MemoryBaseRegister, mZydis.RegName(mem.base));
            prependPlus = true;
        }
        if(mem.index != ZYDIS_REGISTER_NONE) //index register
        {
            if(prependPlus)
                addMemoryOperator('+');
            addToken(TokenType::MemoryIndexRegister, mZydis.RegName(mem.index));
            if(mem.scale > 1)
            {
                addMemoryOperator('*');
                addToken(TokenType::MemoryScale, QString("%1").arg(mem.scale));
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
                valueText = printValue(TokenValue(opsize, duint(mem.disp.value * -1)), false);
            }
            else
                valueText = printValue(value, false);
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
    addToken(TokenType::MemorySegment, printValue(segValue, true), segValue);

    addToken(TokenType::Uncategorized, ":");

    auto offsetValue = TokenValue(mZydis.GetInstr()->info.operand_width / 8, op.ptr.offset);
    addToken(TokenType::Address, printValue(offsetValue, true), offsetValue);

    return true;
}

bool ZydisTokenizer::tokenizeInvalidOperand(const ZydisDecodedOperand & op)
{
    Q_UNUSED(op);
    addToken(TokenType::MnemonicUnusual, "???");
    return true;
}
