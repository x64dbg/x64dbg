#include "Disassembly.h"
#include "Configuration.h"
#include "CodeFolding.h"
#include "EncodeMap.h"
#include "Bridge.h"
#include "CachedFontMetrics.h"
#include "QZydis.h"
#include "MemoryPage.h"
#include "DisassemblyPopup.h"

Disassembly::Disassembly(Architecture* architecture, bool isMain, QWidget* parent)
    : AbstractTableView(parent),
      mArchitecture(architecture),
      mIsMain(isMain)
{
    mMemPage = new MemoryPage(0, 0);

    mInstBuffer.clear();
    setDrawDebugOnly(true);

    historyClear();

    mHighlightToken.text = "";
    mHighlightingMode = false;
    mShowMnemonicBrief = false;

    int maxModuleSize = (int)ConfigUint("Disassembler", "MaxModuleSize");
    Config()->writeUints();

    mDisasm = new QZydis(maxModuleSize, mArchitecture);
    mDisassemblyPopup = new DisassemblyPopup(this, mArchitecture);

    tokenizerConfigUpdatedSlot();
    updateConfigSlot();

    mCodeFoldingManager = nullptr;
    mIsLastInstDisplayed = false;
    mGuiState = Disassembly::NoState;

    // Update fonts immediately because they are used in calculations
    updateFonts();

    setRowCount(mMemPage->getSize());

    addColumnAt(getCharWidth() * 2 * sizeof(dsint) + 8, tr("Address"), false); //address
    addColumnAt(getCharWidth() * 2 * 12 + 8, tr("Bytes"), false); //bytes
    addColumnAt(getCharWidth() * 40, tr("Disassembly"), false); //disassembly
    addColumnAt(getCharWidth() * 40, tr("Mnemonic brief"), false);
    addColumnAt(1000, tr("Comments"), false); //comments
    setColumnHidden(ColMnemonicBrief, true);

    setShowHeader(false); //hide header

    mBackgroundColor = ConfigColor("DisassemblyBackgroundColor");

    mXrefInfo.refcount = 0;

    // Slots
    connect(Bridge::getBridge(), SIGNAL(updateDisassembly()), this, SLOT(reloadData()));
    connect(Bridge::getBridge(), SIGNAL(dbgStateChanged(DBGSTATE)), this, SLOT(debugStateChangedSlot(DBGSTATE)));
    connect(this, SIGNAL(selectionChanged(duint)), this, SLOT(selectionChangedSlot(duint)));
    connect(Config(), SIGNAL(tokenizerConfigUpdated()), this, SLOT(tokenizerConfigUpdatedSlot()));
    connect(Config(), SIGNAL(guiOptionsUpdated()), this, SLOT(updateConfigSlot()));

    Initialize();
}

Disassembly::~Disassembly()
{
    delete mMemPage;
    delete mDisasm;
    if(mXrefInfo.refcount != 0)
        BridgeFree(mXrefInfo.references);
}

Architecture* Disassembly::getArchitecture() const
{
    return mArchitecture;
}

void Disassembly::updateColors()
{
    AbstractTableView::updateColors();
    mBackgroundColor = ConfigColor("DisassemblyBackgroundColor");

    mInstructionHighlightColor = ConfigColor("InstructionHighlightColor");
    mDisassemblyRelocationUnderlineColor = ConfigColor("DisassemblyRelocationUnderlineColor");
    mSelectionColor = ConfigColor("DisassemblySelectionColor");
    mCipBackgroundColor = ConfigColor("DisassemblyCipBackgroundColor");
    mCipColor = ConfigColor("DisassemblyCipColor");
    mBreakpointBackgroundColor = ConfigColor("DisassemblyBreakpointBackgroundColor");
    mBreakpointColor = ConfigColor("DisassemblyBreakpointColor");
    mHardwareBreakpointBackgroundColor = ConfigColor("DisassemblyHardwareBreakpointBackgroundColor");
    mHardwareBreakpointColor = ConfigColor("DisassemblyHardwareBreakpointColor");
    mBookmarkBackgroundColor = ConfigColor("DisassemblyBookmarkBackgroundColor");
    mBookmarkColor = ConfigColor("DisassemblyBookmarkColor");
    mLabelColor = ConfigColor("DisassemblyLabelColor");
    mLabelBackgroundColor = ConfigColor("DisassemblyLabelBackgroundColor");
    mSelectedAddressBackgroundColor = ConfigColor("DisassemblySelectedAddressBackgroundColor");
    mTracedAddressBackgroundColor = ConfigColor("DisassemblyTracedBackgroundColor");
    mSelectedAddressColor = ConfigColor("DisassemblySelectedAddressColor");
    mAddressBackgroundColor = ConfigColor("DisassemblyAddressBackgroundColor");
    mAddressColor = ConfigColor("DisassemblyAddressColor");
    mBytesColor = ConfigColor("DisassemblyBytesColor");
    mBytesBackgroundColor = ConfigColor("DisassemblyBytesBackgroundColor");
    mModifiedBytesColor = ConfigColor("DisassemblyModifiedBytesColor");
    mModifiedBytesBackgroundColor = ConfigColor("DisassemblyModifiedBytesBackgroundColor");
    mRestoredBytesColor = ConfigColor("DisassemblyRestoredBytesColor");
    mRestoredBytesBackgroundColor = ConfigColor("DisassemblyRestoredBytesBackgroundColor");
    mAutoCommentColor = ConfigColor("DisassemblyAutoCommentColor");
    mAutoCommentBackgroundColor = ConfigColor("DisassemblyAutoCommentBackgroundColor");
    mMnemonicBriefColor = ConfigColor("DisassemblyMnemonicBriefColor");
    mMnemonicBriefBackgroundColor = ConfigColor("DisassemblyMnemonicBriefBackgroundColor");
    mCommentColor = ConfigColor("DisassemblyCommentColor");
    mCommentBackgroundColor = ConfigColor("DisassemblyCommentBackgroundColor");
    mUnconditionalJumpLineColor = ConfigColor("DisassemblyUnconditionalJumpLineColor");
    mConditionalJumpLineTrueColor = ConfigColor("DisassemblyConditionalJumpLineTrueColor");
    mConditionalJumpLineFalseColor = ConfigColor("DisassemblyConditionalJumpLineFalseColor");
    mLoopColor = ConfigColor("DisassemblyLoopColor");
    mFunctionColor = ConfigColor("DisassemblyFunctionColor");

    auto a = mSelectionColor, b = mTracedAddressBackgroundColor;
    mTracedSelectedAddressBackgroundColor = QColor((a.red() + b.red()) / 2, (a.green() + b.green()) / 2, (a.blue() + b.blue()) / 2);

    mLoopPen = QPen(mLoopColor, 2);
    mFunctionPen = QPen(mFunctionColor, 2);
    mUnconditionalPen = QPen(mUnconditionalJumpLineColor);
    mConditionalTruePen = QPen(mConditionalJumpLineTrueColor);
    mConditionalFalsePen = QPen(mConditionalJumpLineFalseColor);

    ZydisTokenizer::UpdateColors();
    mDisasm->UpdateConfig();
}

void Disassembly::updateFonts()
{
    setFont(ConfigFont("Disassembly"));
    invalidateCachedFont();
    mTextLayout.setFont(font());
    mTextLayout.setCacheEnabled(true);
}

void Disassembly::updateConfigSlot()
{
    mDisassemblyPopup->setEnabled(!Config()->getBool("Disassembler", "NoBranchDisasmPreview"));
}

void Disassembly::tokenizerConfigUpdatedSlot()
{
    mDisasm->UpdateConfig();
    mPermanentHighlightingMode = ConfigBool("Disassembler", "PermanentHighlightingMode");
    mNoCurrentModuleText = ConfigBool("Disassembler", "NoCurrentModuleText");
}

static void mnemonicBriefRichText(RichTextPainter::List & richText, const Instruction_t & instr, QColor mMnemonicBriefColor, QColor mMnemonicBriefBackgroundColor)
{
    RichTextPainter::CustomRichText_t richBrief;
    richBrief.underline = false;
    richBrief.textColor = mMnemonicBriefColor;
    richBrief.textBackground = mMnemonicBriefBackgroundColor;
    richBrief.flags = RichTextPainter::FlagAll;

    char brief[MAX_STRING_SIZE] = "";
    QString mnem;
    for(const ZydisTokenizer::SingleToken & token : instr.tokens.tokens)
    {
        if(token.type != ZydisTokenizer::TokenType::Space && token.type != ZydisTokenizer::TokenType::Prefix)
        {
            mnem = token.text;
            break;
        }
    }
    if(mnem.isEmpty())
        mnem = instr.instStr;

    int index = mnem.indexOf(' ');
    if(index != -1)
        mnem.truncate(index);
    DbgFunctions()->GetMnemonicBrief(mnem.toUtf8().constData(), MAX_STRING_SIZE, brief);

    QString mnemBrief = brief;
    if(mnemBrief.length())
    {
        RichTextPainter::CustomRichText_t space;
        space.underline = false;
        space.flags = RichTextPainter::FlagNone;
        space.text = " ";
        if(richText.size())
            richText.emplace_back(std::move(space));

        richBrief.text = std::move(mnemBrief);

        richText.emplace_back(std::move(richBrief));
    }
}

#define HANDLE_RANGE_TYPE(prefix, first, last) \
    if(first == prefix ## _BEGIN && last == prefix ## _END) \
        first = prefix ## _SINGLE; \
    if(last == prefix ## _END && first != prefix ## _SINGLE) \
        first = last

/************************************************************************************
                            Reimplemented Functions
************************************************************************************/
/**
 * @brief       This method has been reimplemented. It returns the string to paint or paints it
 *              by its own.
 *
 * @param[in]   painter     Pointer to the painter that allows painting by its own
 * @param[in]   rowBase     Index of the top item (Table offset)
 * @param[in]   rowOffset   Index offset starting from rowBase
 * @param[in]   col         Column index
 * @param[in]   x           Rectangle x
 * @param[in]   y           Rectangle y
 * @param[in]   w           Rectangle width
 * @param[in]   h           Rectangle heigth
 *
 * @return      String to paint.
 */
QString Disassembly::paintContent(QPainter* painter, duint row, duint col, int x, int y, int w, int h)
{
    auto rowOffset = row - getTableOffset();

    if(mHighlightingMode)
    {
        QPen pen(Qt::red);
        pen.setWidth(2);
        painter->setPen(pen);
        QRect rect = viewport()->rect();
        rect.adjust(1, 1, -1, -1);
        painter->drawRect(rect);
    }
    bool instSelected = isSelected(&mInstBuffer, rowOffset);
    auto va = rvaToVa(mInstBuffer.at(rowOffset).rva);
    auto traceCount = DbgFunctions()->GetTraceRecordHitCount(va);

    // Highlight if selected
    if(instSelected && traceCount)
        painter->fillRect(QRect(x, y, w, h), QBrush(mTracedSelectedAddressBackgroundColor));
    else if(instSelected)
        painter->fillRect(QRect(x, y, w, h), QBrush(mSelectionColor));
    else if(traceCount)
    {
        // Color depending on how often a sequence of code is executed
        int exponent = 1;
        while(traceCount >>= 1) //log2(traceCount)
            exponent++;
        int colorDiff = (exponent * exponent) / 2;

        // If the user has a light trace background color, substract
        if(mTracedAddressBackgroundColor.blue() > 160)
            colorDiff *= -1;

        painter->fillRect(QRect(x, y, w, h),
                          QBrush(QColor(mTracedAddressBackgroundColor.red(),
                                        mTracedAddressBackgroundColor.green(),
                                        std::max(0, std::min(256, mTracedAddressBackgroundColor.blue() + colorDiff)))));
    }

    switch(col)
    {
    case ColAddress: // Draw address (+ label)
    {
        RichTextPainter::CustomRichText_t richText;
        richText.underline = false;
        richText.textColor = mTextColor;
        richText.flags = RichTextPainter::FlagColor;

        QString label;
        QString addrText = getAddrText(va, label);
        richText.text = addrText;
        BPXTYPE bpxtype = DbgGetBpxTypeAt(va);
        bool isbookmark = DbgGetBookmarkAt(va);
        if(rvaToVa(mInstBuffer.at(rowOffset).rva) == mCipVa && !Bridge::getBridge()->mIsRunning && DbgMemFindBaseAddr(DbgValFromString("cip"), nullptr)) //cip + not running + valid cip
        {
            richText.textBackground = mCipBackgroundColor;
            if(!isbookmark) //no bookmark
            {
                if(bpxtype & bp_normal) //normal breakpoint
                {
                    QColor bpColor = mBreakpointBackgroundColor;
                    if(!bpColor.alpha()) //we don't want transparent text
                        bpColor = mBreakpointColor;
                    if(bpColor == mCipBackgroundColor)
                        bpColor = mCipColor;
                    richText.textColor = bpColor;
                }
                else if(bpxtype & bp_hardware) //hardware breakpoint only
                {
                    QColor hwbpColor = mHardwareBreakpointBackgroundColor;
                    if(!hwbpColor.alpha()) //we don't want transparent text
                        hwbpColor = mHardwareBreakpointColor;
                    if(hwbpColor == mCipBackgroundColor)
                        hwbpColor = mCipColor;
                    richText.textColor = hwbpColor;
                }
                else //no breakpoint
                {
                    richText.textColor = mCipColor;
                }
            }
            else //bookmark
            {
                QColor bookmarkColor = mBookmarkBackgroundColor;
                if(!bookmarkColor.alpha()) //we don't want transparent text
                    bookmarkColor = mBookmarkColor;
                if(bookmarkColor == mCipBackgroundColor)
                    bookmarkColor = mCipColor;
                richText.textColor = bookmarkColor;
            }
        }
        else //non-cip address
        {
            if(!isbookmark) //no bookmark
            {
                if(!label.isEmpty()) //label
                {
                    if(bpxtype == bp_none) //label only : fill label background
                    {
                        richText.textColor = mLabelColor;
                        richText.textBackground = mLabelBackgroundColor;
                    }
                    else //label + breakpoint
                    {
                        if(bpxtype & bp_normal) //label + normal breakpoint
                        {
                            richText.textColor = mBreakpointColor;
                            richText.textBackground = mBreakpointBackgroundColor;
                        }
                        else if(bpxtype & bp_hardware) //label + hardware breakpoint only
                        {
                            richText.textColor = mHardwareBreakpointColor;
                            richText.textBackground = mHardwareBreakpointBackgroundColor;
                        }
                        else //other cases -> do as normal
                        {
                            richText.textColor = mLabelColor;
                            richText.textBackground = mLabelBackgroundColor;
                        }
                    }
                }
                else //no label
                {
                    if(bpxtype == bp_none) //no label, no breakpoint
                    {
                        if(instSelected)
                        {
                            richText.textColor = mSelectedAddressColor;
                            richText.textBackground = mSelectedAddressBackgroundColor;
                        }
                        else
                        {
                            richText.textColor = mAddressColor;
                            richText.textBackground = mAddressBackgroundColor;
                        }
                    }
                    else //breakpoint only
                    {
                        if(bpxtype & bp_normal) //normal breakpoint
                        {
                            richText.textColor = mBreakpointColor;
                            richText.textBackground = mBreakpointBackgroundColor;
                        }
                        else if(bpxtype & bp_hardware) //hardware breakpoint only
                        {
                            richText.textColor = mHardwareBreakpointColor;
                            richText.textBackground = mHardwareBreakpointBackgroundColor;
                        }
                        else //other cases (memory breakpoint in disassembly) -> do as normal
                        {
                            if(instSelected)
                            {
                                richText.textColor = mSelectedAddressColor;
                                richText.textBackground = mSelectedAddressBackgroundColor;
                            }
                            else
                            {
                                richText.textColor = mAddressColor;
                                richText.textBackground = mAddressBackgroundColor;
                            }
                        }
                    }
                }
            }
            else //bookmark
            {
                if(!label.isEmpty()) //label + bookmark
                {
                    if(bpxtype == bp_none) //label + bookmark
                    {
                        richText.textColor = mLabelColor;
                        richText.textBackground = mBookmarkBackgroundColor;
                    }
                    else //label + breakpoint + bookmark
                    {
                        QColor color = mBookmarkBackgroundColor;
                        if(!color.alpha()) //we don't want transparent text
                            color = mAddressColor;
                        richText.textColor = color;
                        if(bpxtype & bp_normal) //label + bookmark + normal breakpoint
                        {
                            richText.textBackground = mBreakpointBackgroundColor;
                        }
                        else if(bpxtype & bp_hardware) //label + bookmark + hardware breakpoint only
                        {
                            richText.textBackground = mHardwareBreakpointBackgroundColor;
                        }
                    }
                }
                else //bookmark, no label
                {
                    if(bpxtype == bp_none) //bookmark only
                    {
                        richText.textColor = mBookmarkColor;
                        richText.textBackground = mBookmarkBackgroundColor;
                    }
                    else //bookmark + breakpoint
                    {
                        QColor color = mBookmarkBackgroundColor;
                        if(!color.alpha()) //we don't want transparent text
                            color = mAddressColor;
                        richText.textColor = color;
                        if(bpxtype & bp_normal) //bookmark + normal breakpoint
                        {
                            richText.textBackground = mBreakpointBackgroundColor;
                        }
                        else if(bpxtype & bp_hardware) //bookmark + hardware breakpoint only
                        {
                            richText.textBackground = mHardwareBreakpointBackgroundColor;
                        }
                        else //other cases (bookmark + memory breakpoint in disassembly) -> do as normal
                        {
                            richText.textColor = mBookmarkColor;
                            richText.textBackground = mBookmarkBackgroundColor;
                        }
                    }
                }
            }
        }

        if(richText.textBackground.alpha())
        {
            painter->fillRect(QRect(x, y, w, h), richText.textBackground);
        }

        RichTextPainter::List list;
        list.emplace_back(std::move(richText));
        paintRichText(x, y, w, h, 2, std::move(list), rowOffset, col);
    }
    break;

    case ColBytes: //draw bytes
    {
        const Instruction_t & instr = mInstBuffer.at(rowOffset);
        //draw functions
        Function_t funcType;
        FUNCTYPE funcFirst = DbgGetFunctionTypeAt(va);
        FUNCTYPE funcLast = DbgGetFunctionTypeAt(va + instr.length - 1);
        HANDLE_RANGE_TYPE(FUNC, funcFirst, funcLast);
        switch(funcFirst)
        {
        case FUNC_SINGLE:
            funcType = Function_single;
            break;
        case FUNC_NONE:
            funcType = Function_none;
            break;
        case FUNC_BEGIN:
            funcType = Function_start;
            break;
        case FUNC_MIDDLE:
            funcType = Function_middle;
            break;
        case FUNC_END:
            funcType = Function_end;
            break;
        }
        int funcsize = paintFunctionGraphic(painter, x, y, funcType, false);

        painter->setPen(mFunctionPen);

        XREFTYPE refType = DbgGetXrefTypeAt(va);
        char indicator;
        if(refType == XREF_JMP)
        {
            indicator = '>';
        }
        else if(refType == XREF_CALL)
        {
            indicator = '$';
        }
        else if(funcType != Function_none)
        {
            indicator = '.';
        }
        else
        {
            indicator = ' ';
        }

        int charwidth = getCharWidth();
        painter->drawText(QRect(x + funcsize, y, charwidth, h), Qt::AlignVCenter | Qt::AlignLeft, QString(indicator));
        funcsize += charwidth;

        //draw jump arrows
        int jumpsize = paintJumpsGraphic(painter, x + funcsize, y - 1, mInstBuffer.at(rowOffset)); //jump line

        //draw bytes
        auto richBytes = getRichBytes(instr, instSelected);
        paintRichText(x, y, w, h, jumpsize + funcsize, std::move(richBytes), rowOffset, col);
    }
    break;

    case ColDisassembly: //draw disassembly (with colours needed)
    {
        int loopsize = 0;
        int depth = 0;

        while(1) //paint all loop depths
        {
            LOOPTYPE loopFirst = DbgGetLoopTypeAt(va, depth);
            LOOPTYPE loopLast = DbgGetLoopTypeAt(va + mInstBuffer.at(rowOffset).length - 1, depth);
            HANDLE_RANGE_TYPE(LOOP, loopFirst, loopLast);
            if(loopFirst == LOOP_NONE)
                break;
            Function_t funcType;
            switch(loopFirst)
            {
            case LOOP_SINGLE:
                funcType = Function_single;
                break;
            case LOOP_BEGIN:
                funcType = Function_start;
                break;
            case LOOP_ENTRY:
                funcType = Function_loop_entry;
                break;
            case LOOP_MIDDLE:
                funcType = Function_middle;
                break;
            case LOOP_END:
                funcType = Function_end;
                break;
            default:
                break;
            }
            loopsize += paintFunctionGraphic(painter, x + loopsize, y, funcType, loopFirst != LOOP_SINGLE);
            depth++;
        }

        RichTextPainter::List richText;
        auto & token = mInstBuffer[rowOffset].tokens;
        if(mHighlightToken.text.length())
            ZydisTokenizer::TokenToRichText(token, richText, &mHighlightToken);
        else
            ZydisTokenizer::TokenToRichText(token, richText, 0);
        int xinc = 4 + loopsize;
        paintRichText(x, y, w, h, xinc, std::move(richText), rowOffset, col);
        token.x = x + loopsize + xinc;
    }
    break;

    case ColMnemonicBrief: //mnemonic brief
    {
        RichTextPainter::List richText;
        if(mShowMnemonicBrief)
        {
            mnemonicBriefRichText(richText, mInstBuffer.at(rowOffset), mMnemonicBriefColor, mMnemonicBriefBackgroundColor);
        }
        paintRichText(x, y, w, h, 3, std::move(richText), rowOffset, col);
    }
    break;

    case ColComment: //draw comments
    {
        //draw arguments
        Function_t funcType;
        ARGTYPE argFirst = DbgGetArgTypeAt(va);
        ARGTYPE argLast = DbgGetArgTypeAt(va + mInstBuffer.at(rowOffset).length - 1);
        HANDLE_RANGE_TYPE(ARG, argFirst, argLast);
        switch(argFirst)
        {
        case ARG_SINGLE:
            funcType = Function_single;
            break;
        case ARG_NONE:
            funcType = Function_none;
            break;
        case ARG_BEGIN:
            funcType = Function_start;
            break;
        case ARG_MIDDLE:
            funcType = Function_middle;
            break;
        case ARG_END:
            funcType = Function_end;
            break;
        }
        RichTextPainter::List richText;
        int argsize = funcType == Function_none ? 3 : paintFunctionGraphic(painter, x, y, funcType, false);

        RichTextPainter::CustomRichText_t richComment;
        richComment.underline = false;
        richComment.textColor = mTextColor;
        richComment.textBackground = mBackgroundColor;
        richComment.flags = RichTextPainter::FlagAll;

        QString comment;
        bool autoComment = false;
        char label[MAX_LABEL_SIZE] = "";
        if(GetCommentFormat(va, comment, &autoComment))
        {
            if(autoComment)
            {
                richComment.textColor = mAutoCommentColor;
                richComment.textBackground = mAutoCommentBackgroundColor;
            }
            else //user comment
            {
                richComment.textColor = mCommentColor;
                richComment.textBackground = mCommentBackgroundColor;
            }

            richComment.text = std::move(comment);
            richText.emplace_back(std::move(richComment));
        }
        else if(DbgGetLabelAt(va, SEG_DEFAULT, label)) // label but no comment
        {
            richComment.textColor = mLabelColor;
            richComment.textBackground = mLabelBackgroundColor;
            richComment.text = label;
            richText.emplace_back(std::move(richComment));
        }

        if(mShowMnemonicBrief && getColumnHidden(ColMnemonicBrief))
        {
            mnemonicBriefRichText(richText, mInstBuffer.at(rowOffset), mMnemonicBriefColor, mMnemonicBriefBackgroundColor);
        }

        paintRichText(x, y, w, h, argsize, std::move(richText), rowOffset, col);
    }
    break;
    }
    return QString();
}

/************************************************************************************
                            Mouse Management
************************************************************************************/
/**
 * @brief       This method has been reimplemented. It manages the following actions:
 *               - Multi-rows selection
 *
 * @param[in]   event       Mouse event
 *
 * @return      Nothing.
 */
void Disassembly::mouseMoveEvent(QMouseEvent* event)
{
    //qDebug() << "Disassembly::mouseMoveEvent";

    int y = event->y();

    if(mGuiState == Disassembly::MultiRowsSelectionState)
    {
        //qDebug() << "State = MultiRowsSelectionState";

        if((transY(y) >= 0) && (transY(y) <= this->getTableHeight()))
        {
            auto i = getIndexOffsetFromY(transY(y));

            if(mMemPage->getSize() > 0)
            {
                // Bound
                i = i >= mInstBuffer.size() ? mInstBuffer.size() - 1 : i;
                i = i < 0 ? 0 : i;

                if(i >= mInstBuffer.size())
                    return;
                auto rowIndex = mInstBuffer.at(i).rva;
                auto instrSize = getInstructionRVA(rowIndex, 1) - rowIndex - 1;

                if(rowIndex < getRowCount())
                {
                    setSingleSelection(getInitialSelection());
                    expandSelectionUpTo(getInstructionRVA(getInitialSelection(), 1) - 1);
                    if(rowIndex > getInitialSelection()) //select down
                        expandSelectionUpTo(rowIndex + instrSize);
                    else
                        expandSelectionUpTo(rowIndex);

                    emit selectionExpanded();

                    // TODO: only update if the selection actually changed
                    updateViewport();

                    return;
                }
            }
        }
        else if(y > this->height())
        {
            verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepAdd);
        }
        else if(transY(y) < 0)
        {
            verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepSub);
        }
    }

    AbstractTableView::mouseMoveEvent(event);
}

duint Disassembly::getAddressForPosition(int mousex, int mousey)
{
    if(mHighlightingMode)
        return 0; //Don't show this in highlight mode
    if(getColumnIndexFromX(mousex) != 2)
        return 0; //Disassembly popup for other column is undefined
    auto rowOffset = getIndexOffsetFromY(transY(mousey));
    if(rowOffset < mInstBuffer.size())
    {
        ZydisTokenizer::SingleToken token;
        auto & instruction = mInstBuffer.at(rowOffset);
        if(ZydisTokenizer::TokenFromX(instruction.tokens, token, mousex, mFontMetrics))
        {
            duint addr = token.value.value;
            bool isCodePage = DbgFunctions()->MemIsCodePage(addr, false);
            if(!isCodePage && instruction.branchDestination)
            {
                addr = instruction.branchDestination;
                isCodePage = DbgFunctions()->MemIsCodePage(addr, false);
            }
            if(isCodePage && (addr - mMemPage->getBase() < mInstBuffer.front().rva || addr - mMemPage->getBase() > mInstBuffer.back().rva))
            {
                return addr;
            }
        }
    }
    return 0;
}

/**
 * @brief       This method has been reimplemented. It manages the following actions:
 *               - Multi-rows selection
 *
 * @param[in]   event       Mouse event
 *
 * @return      Nothing.
 */
void Disassembly::mousePressEvent(QMouseEvent* event)
{
    bool accept = false;

    if(mHighlightingMode || mPermanentHighlightingMode)
    {
        if(getColumnIndexFromX(event->x()) == 2) //click in instruction column
        {
            auto rowOffset = getIndexOffsetFromY(transY(event->y()));
            if(rowOffset < mInstBuffer.size())
            {
                ZydisTokenizer::SingleToken token;
                if(ZydisTokenizer::TokenFromX(mInstBuffer.at(rowOffset).tokens, token, event->x(), mFontMetrics))
                {
                    if(ZydisTokenizer::IsHighlightableToken(token))
                    {
                        if(!ZydisTokenizer::TokenEquals(&token, &mHighlightToken) || event->button() == Qt::RightButton)
                            mHighlightToken = token;
                        else
                            mHighlightToken = ZydisTokenizer::SingleToken();
                    }
                    else if(!mPermanentHighlightingMode)
                    {
                        mHighlightToken = ZydisTokenizer::SingleToken();
                    }
                }
                else if(!mPermanentHighlightingMode)
                {
                    mHighlightToken = ZydisTokenizer::SingleToken();
                }
            }
        }
        else if(!mPermanentHighlightingMode)
        {
            mHighlightToken = ZydisTokenizer::SingleToken();
        }
        if(!mPermanentHighlightingMode)
            return;
    }

    if(DbgIsDebugging() && ((event->buttons() & Qt::LeftButton) != 0) && ((event->buttons() & Qt::RightButton) == 0))
    {
        if(getGuiState() == AbstractTableView::NoState)
        {
            if(event->y() > getHeaderHeight())
            {
                auto index = getIndexOffsetFromY(transY(event->y()));

                if(mInstBuffer.size() > index && index >= 0)
                {
                    auto rowIndex = mInstBuffer.at(index).rva;
                    auto instrSize = mInstBuffer.at(index).length - 1;
                    if(!(event->modifiers() & Qt::ShiftModifier)) //SHIFT pressed
                        setSingleSelection(rowIndex);
                    if(getSelectionStart() > rowIndex) //select up
                    {
                        setSingleSelection(getInitialSelection());
                        expandSelectionUpTo(getInstructionRVA(getInitialSelection(), 1) - 1);
                        expandSelectionUpTo(rowIndex);
                    }
                    else //select down
                    {
                        setSingleSelection(getInitialSelection());
                        expandSelectionUpTo(rowIndex + instrSize);
                    }

                    mGuiState = Disassembly::MultiRowsSelectionState;

                    // TODO: only update if the selection actually changed
                    updateViewport();

                    accept = true;
                }
            }
        }
    }

    if(!accept)
        AbstractTableView::mousePressEvent(event);
}

/**
 * @brief       This method has been reimplemented. It manages the following actions:
 *               - Multi-rows selection
 *
 * @param[in]   event       Mouse event
 *
 * @return      Nothing.
 */
void Disassembly::mouseReleaseEvent(QMouseEvent* event)
{
    bool accept = true;

    if((event->buttons() & Qt::LeftButton) == 0)
    {
        if(mGuiState == Disassembly::MultiRowsSelectionState)
        {
            mGuiState = Disassembly::NoState;

            accept = false;
        }
    }
    if((event->button() & Qt::BackButton) != 0)
    {
        accept = true;
        historyPrevious();
    }
    else if((event->button() & Qt::ForwardButton) != 0)
    {
        accept = true;
        historyNext();
    }

    if(accept)
        AbstractTableView::mouseReleaseEvent(event);
}

void Disassembly::wheelEvent(QWheelEvent* event)
{
    if(event->modifiers() == Qt::NoModifier)
        AbstractTableView::wheelEvent(event);
    else if(event->modifiers() == Qt::ControlModifier) // Zoom
        Config()->zoomFont("Disassembly", event);
}

/************************************************************************************
                            Keyboard Management
************************************************************************************/
/**
 * @brief       This method has been reimplemented. It processes the Up/Down key events.
 *
 * @param[in]   event       Key event
 *
 * @return      Nothing.
 */
void Disassembly::keyPressEvent(QKeyEvent* event)
{
    int key = event->key();

    if(event->modifiers() == (Qt::ControlModifier | Qt::AltModifier))
    {
        if(key == Qt::Key_Left)
        {
            setTableOffset(getTableOffset() - 1);
        }
        else if(key == Qt::Key_Right)
        {
            setTableOffset(getTableOffset() + 1);
        }

        // TODO: only update if the selection actually changed
        updateViewport();
    }
    else if(key == Qt::Key_Up || key == Qt::Key_Down)
    {
        dsint botRVA = getTableOffset();
        dsint topRVA = getInstructionRVA(getTableOffset(), getNbrOfLineToPrint() - 1);

        bool expand = false;
        if(event->modifiers() & Qt::ShiftModifier) //SHIFT pressed
            expand = true;

        dsint initialStart = getSelectionStart();

        if(key == Qt::Key_Up)
            selectPrevious(expand); //TODO: fix this shit to actually go to whatever the previous instruction shows
        else
            selectNext(expand);

        bool expandedUp = initialStart != getSelectionStart();
        dsint modifiedSelection = expandedUp ? getSelectionStart() : getSelectionEnd();

        if(modifiedSelection < botRVA)
        {
            setTableOffset(modifiedSelection);
        }
        else if(modifiedSelection >= topRVA)
        {
            setTableOffset(getInstructionRVA(modifiedSelection, -(dsint)getNbrOfLineToPrint() + 2));
        }

        // TODO: only update if the selection actually changed
        updateViewport();
    }
    else if(key == Qt::Key_Return || key == Qt::Key_Enter)
    {
        // Follow branch instruction
        duint dest = DbgGetBranchDestination(rvaToVa(getInitialSelection()));
        if(DbgMemIsValidReadPtr(dest))
        {
            gotoAddress(dest);
            return;
        }
#ifdef X64DBG
        // Follow memory operand in dump
        DISASM_INSTR instr;
        DbgDisasmAt(rvaToVa(getInitialSelection()), &instr);
        for(int op = instr.argcount - 1; op >= 0; op--)
        {
            if(instr.arg[op].type == arg_memory)
            {
                dest = instr.arg[op].value;
                if(DbgMemIsValidReadPtr(dest))
                {
                    if(instr.arg[op].segment == SEG_SS)
                        DbgCmdExec(QString("sdump %1").arg(ToPtrString(dest)));
                    else
                        DbgCmdExec(QString("dump %1").arg(ToPtrString(dest)));
                    return;
                }
            }
        }
        // Follow constant in dump
        for(int op = instr.argcount - 1; op >= 0; op--)
        {
            if(instr.arg[op].type == arg_normal)
            {
                dest = instr.arg[op].value;
                if(DbgMemIsValidReadPtr(dest))
                {
                    DbgCmdExec(QString("dump %1").arg(ToPtrString(dest)));
                    return;
                }
            }
        }
#endif // X64DBG
    }
    else
        AbstractTableView::keyPressEvent(event);
}

/************************************************************************************
                            ScrollBar Management
 ***********************************************************************************/
/**
 * @brief       This method has been reimplemented. It realigns the slider on real instructions except
 *              when the type is QAbstractSlider::SliderNoAction. This type (QAbstractSlider::SliderNoAction)
 *              is used to force the disassembling at a specific RVA.
 *
 * @param[in]   type      Type of action
 * @param[in]   value     Old table offset
 * @param[in]   delta     Scrollbar value delta compared to the previous state
 *
 * @return      Return the value of the new table offset.
 */
duint Disassembly::sliderMovedHook(QScrollBar::SliderAction action, duint value, dsint delta)
{
    // QAbstractSlider::SliderNoAction is used to disassemble at a specific address
    if(action == QAbstractSlider::SliderNoAction)
        return value + delta;

    // If it's a slider action, disassemble one instruction back and one instruction next in order to be aligned on a real instruction
    if(action == QAbstractSlider::SliderMove)
    {
        dsint newValue = 0;

        if(value + delta > 0)
        {
            newValue = getInstructionRVA(value + delta, -1);
            newValue = getInstructionRVA(newValue, 1);
        }

        return newValue;
    }

    // For other actions, disassemble according to the delta
    return getInstructionRVA(value, delta);
}


/************************************************************************************
                            Jumps Graphic
************************************************************************************/
/**
 * @brief       This method paints the graphic for jumps.
 *
 * @param[in]   painter     Pointer to the painter that allows painting by its own
 * @param[in]   x           Rectangle x
 * @param[in]   y           Rectangle y
 * @param[in]   addr        RVA of address to process
 *
 * @return      Nothing.
 */
int Disassembly::paintJumpsGraphic(QPainter* painter, int x, int y, const Instruction_t & instruction)
{
    auto isjmp = instruction.branchType != Instruction_t::None && instruction.branchType != Instruction_t::Call;
    auto selHeadRVA = mSelection.fromIndex;
    auto rva = instruction.rva;
    auto curVa = rvaToVa(rva);
    auto selVa = rvaToVa(mSelection.firstSelectedIndex);

    if(mSelectedInstruction.rva != selHeadRVA)
    {
        mSelectedInstruction = DisassembleAt(selHeadRVA);
    }

    auto branchType = mSelectedInstruction.branchType;

    bool showXref = false;

    GraphicDump pict = GD_Nothing;

    if(branchType != Instruction_t::None && branchType != Instruction_t::Call)
    {
        if(mMemPage->inRange(mSelectedInstruction.branchDestination))
        {
            auto destRVA = mSelectedInstruction.branchDestination - mMemPage->getBase();

            if(destRVA < selHeadRVA)
            {
                if(rva == destRVA)
                    pict = GD_HeadFromBottom;
                else if(rva > destRVA && rva < selHeadRVA)
                    pict = GD_Vert;
                else if(rva == selHeadRVA)
                    pict = GD_FootToTop;
            }
            else if(destRVA > selHeadRVA)
            {
                if(rva == selHeadRVA)
                    pict = GD_FootToBottom;
                else if(rva > selHeadRVA && rva < destRVA)
                    pict = GD_Vert;
                else if(rva == destRVA)
                    pict = GD_HeadFromTop;
            }
        }
    }
    else if(mXrefInfo.refcount > 0)
    {
        // TODO: bad performance for sure, this code is also doing things in a super weird order...
        duint max = selVa, min = selVa;
        showXref = true;
        int jmpcount = 0;
        for(duint i = 0; i < mXrefInfo.refcount; i++)
        {
            if(mXrefInfo.references[i].type != XREF_JMP)
                continue;
            jmpcount++;
            if(curVa == mXrefInfo.references[i].addr)
                pict = GD_VertHori;
            if(mXrefInfo.references[i].addr > max)
                max = mXrefInfo.references[i].addr;
            if(mXrefInfo.references[i].addr < min)
                min = mXrefInfo.references[i].addr;
        }
        if(jmpcount)
        {
            if(curVa == selVa)
            {
                if(max == selVa)
                {
                    pict = GD_HeadFromTop;
                }
                else if(min == selVa)
                {
                    pict = GD_HeadFromBottom;
                }
                else if(max > selVa && min < selVa)
                {
                    pict = GD_HeadFromBoth;
                }

            }
            else if(curVa < selVa && curVa == min)
            {
                pict =  GD_FootToBottom;
            }
            else if(curVa > selVa && curVa == max)
            {
                pict = GD_FootToTop;
            }
            if(pict == GD_Nothing && curVa > min && curVa < max)
                pict = GD_Vert;
        }
    }

    GraphicJumpDirection curInstDir = GJD_Nothing;

    if(isjmp)
    {
        duint curInstDestination = instruction.branchDestination;
        if(curInstDestination == 0 || curVa == curInstDestination)
        {
            curInstDir = GJD_Nothing;
        }
        else if(curInstDestination >= mMemPage->getBase() && curInstDestination < mMemPage->getBase() + mMemPage->getSize())
        {
            if(curInstDestination < curVa)
            {
                curInstDir = GJD_Up;
            }
            else
            {
                curInstDir = GJD_Down;
            }
        }
        else
        {
            curInstDir = GJD_Out;
        }
    }

    int halfRow = getRowHeight() / 2 + 1;

    painter->setPen(mConditionalTruePen);
    switch(curInstDir)
    {
    case GJD_Nothing:
    {
    }
    break;

    case GJD_Up:
    {
        QPoint points[] =
        {
            QPoint(x, y + halfRow + 1),
            QPoint(x + 2, y + halfRow - 1),
            QPoint(x + 4, y + halfRow + 1),
        };

        painter->drawPolyline(points, 3);
    }
    break;

    case GJD_Down:
    {
        QPoint points[] =
        {
            QPoint(x, y + halfRow - 1),
            QPoint(x + 2, y + halfRow + 1),
            QPoint(x + 4, y + halfRow - 1),
        };

        painter->drawPolyline(points, 3);
    }
    break;

    case GJD_Out:
    {
        QPoint points[] =
        {
            QPoint(x, y + halfRow),
            QPoint(x + 4, y + halfRow),
        };

        painter->drawPolyline(points, 2);
    }
    break;
    }

    x += 8;

    if(showXref)
    {
        painter->setPen(mUnconditionalPen);
    }
    else
    {
        bool bIsExecute = DbgIsJumpGoingToExecute(rvaToVa(mSelectedInstruction.rva));

        if(branchType == Instruction_t::Unconditional) //unconditional
        {
            painter->setPen(mUnconditionalPen);
        }
        else
        {
            if(bIsExecute)
                painter->setPen(mConditionalTruePen);
            else
                painter->setPen(mConditionalFalsePen);
        }
    }

    if(pict == GD_Vert)
    {
        painter->drawLine(x, y, x, y + getRowHeight());
    }
    else if(pict == GD_FootToBottom)
    {
        painter->drawLine(x, y + halfRow, x + 5, y + halfRow);
        painter->drawLine(x, y + halfRow, x, y + getRowHeight());
    }
    else if(pict == GD_FootToTop)
    {
        painter->drawLine(x, y + halfRow, x + 5, y + halfRow);
        painter->drawLine(x, y, x, y + halfRow);
    }
    else if(pict == GD_HeadFromBottom)
    {
        QPoint points[] =
        {
            QPoint(x + 3, y + halfRow - 2),
            QPoint(x + 5, y + halfRow),
            QPoint(x + 3, y + halfRow + 2),
        };

        painter->drawLine(x, y + halfRow, x + 5, y + halfRow);
        painter->drawLine(x, y + halfRow, x, y + getRowHeight());
        painter->drawPolyline(points, 3);
    }
    else if(pict == GD_HeadFromTop)
    {
        QPoint points[] =
        {
            QPoint(x + 3, y + halfRow - 2),
            QPoint(x + 5, y + halfRow),
            QPoint(x + 3, y + halfRow + 2),
        };

        painter->drawLine(x, y + halfRow, x + 5, y + halfRow);
        painter->drawLine(x, y, x, y + halfRow);
        painter->drawPolyline(points, 3);
    }
    else if(pict == GD_HeadFromBoth)
    {
        QPoint points[] =
        {
            QPoint(x + 3, y + halfRow - 2),
            QPoint(x + 5, y + halfRow),
            QPoint(x + 3, y + halfRow + 2),
        };

        painter->drawLine(x, y + halfRow, x + 5, y + halfRow);
        painter->drawLine(x, y, x, y + getRowHeight());
        painter->drawPolyline(points, 3);
    }
    else if(pict == GD_VertHori)
    {
        painter->drawLine(x, y + halfRow, x + 5, y + halfRow);
        painter->drawLine(x, y, x, y + getRowHeight());
    }

    return 15;
}

/************************************************************************************
                            Function Graphic
************************************************************************************/
/**
 * @brief       This method paints the graphic for functions/loops.
 *
 * @param[in]   painter     Pointer to the painter that allows painting by its own
 * @param[in]   x           Rectangle x
 * @param[in]   y           Rectangle y
 * @param[in]   funcType    Type of drawing to make
 *
 * @return      Width of the painted data.
 */

int Disassembly::paintFunctionGraphic(QPainter* painter, int x, int y, Function_t funcType, bool loop)
{
    if(loop && funcType == Function_none)
        return 0;
    if(loop)
        painter->setPen(mLoopPen); //thick black line
    else
        painter->setPen(mFunctionPen); //thick black line
    int height = getRowHeight();
    int x_add = 5;
    int y_add = 4;
    int end_add = 2;
    int line_width = 3;
    if(loop)
    {
        end_add = -1;
        x_add = 4;
    }
    switch(funcType)
    {
    case Function_single:
    {
        if(loop)
            y_add = height / 2 + 1;
        painter->drawLine(x + x_add + line_width, y + y_add, x + x_add, y + y_add);
        painter->drawLine(x + x_add, y + y_add, x + x_add, y + height - y_add - 1);
        if(loop)
            y_add = height / 2 - 1;
        painter->drawLine(x + x_add, y + height - y_add, x + x_add + line_width, y + height - y_add);
    }
    break;

    case Function_start:
    {
        if(loop)
            y_add = height / 2 + 1;
        painter->drawLine(x + x_add + line_width, y + y_add, x + x_add, y + y_add);
        painter->drawLine(x + x_add, y + y_add, x + x_add, y + height);
    }
    break;

    case Function_middle:
    {
        painter->drawLine(x + x_add, y, x + x_add, y + height);
    }
    break;

    case Function_loop_entry:
    {
        int trisize = 2;
        int y_start = (height - trisize * 2) / 2 + y;
        painter->drawLine(x + x_add, y_start, x + trisize + x_add, y_start + trisize);
        painter->drawLine(x + trisize + x_add, y_start + trisize, x + x_add, y_start + trisize * 2);

        painter->drawLine(x + x_add, y, x + x_add, y_start - 1);
        painter->drawLine(x + x_add, y_start + trisize * 2 + 2, x + x_add, y + height);
    }
    break;

    case Function_end:
    {
        if(loop)
            y_add = height / 2 - 1;
        painter->drawLine(x + x_add, y, x + x_add, y + height - y_add);
        painter->drawLine(x + x_add, y + height - y_add, x + x_add + line_width, y + height - y_add);
    }
    break;

    case Function_none:
    {

    }
    break;
    }
    return x_add + line_width + end_add;
}

void Disassembly::paintRichText(int x, int y, int w, int h, int xinc, const RichTextPainter::List & richText, int rowOffset, int column)
{
    RichTextInfo & info = mRichText[column][rowOffset];
    info.x = x;
    info.y = y;
    info.w = w;
    info.h = h;
    info.xinc = xinc;
    info.richText = richText;
    info.alive = true;
}

void Disassembly::paintRichText(int x, int y, int w, int h, int xinc, RichTextPainter::List && richText, int rowOffset, int column)
{
    RichTextInfo & info = mRichText[column][rowOffset];
    info.x = x;
    info.y = y;
    info.w = w;
    info.h = h;
    info.xinc = xinc;
    info.richText = std::move(richText);
    info.alive = true;
}

/************************************************************************************
                            Instructions Management
 ***********************************************************************************/
/**
 * @brief       Returns the RVA of count-th instructions before the given instruction RVA.
 *
 * @param[in]   rva         Instruction RVA
 * @param[in]   count       Instruction count
 *
 * @return      RVA of count-th instructions before the given instruction RVA.
 */
duint Disassembly::getPreviousInstructionRVA(duint rva, duint count)
{
    QByteArray buffer;

    // TODO: explicitly bail out early
    dsint bottomByteRealRVA = (dsint)rva - 16 * (count + 3);
    if(mCodeFoldingManager)
    {
        if(mCodeFoldingManager->isFolded(rvaToVa(bottomByteRealRVA)))
        {
            bottomByteRealRVA = mCodeFoldingManager->getFoldBegin(bottomByteRealRVA) - mMemPage->getBase() - 16 * (count + 3);
        }
    }
    bottomByteRealRVA = bottomByteRealRVA < 0 ? 0 : bottomByteRealRVA;

    auto virtualRVA = (dsint)rva - bottomByteRealRVA;

    auto maxByteCountToRead = virtualRVA + 1 + 16;
    buffer.resize(maxByteCountToRead);

    mMemPage->read(buffer.data(), bottomByteRealRVA, buffer.size());

    dsint addr = mDisasm->DisassembleBack((uint8_t*)buffer.data(), rvaToVa(bottomByteRealRVA), buffer.size(), virtualRVA, count);

    addr += rva - virtualRVA;

    return addr;
}

/**
 * @brief       Returns the RVA of count-th instructions after the given instruction RVA.
 *
 * @param[in]   rva         Instruction RVA
 * @param[in]   count       Instruction count
 * @param[in]   isGlobal    Whether it rejects rva beyond current page
 *
 * @return      RVA of count-th instructions after the given instruction RVA.
 */
duint Disassembly::getNextInstructionRVA(duint rva, duint count, bool isGlobal)
{
    QByteArray buffer;

    duint maxByteCountToRead = 0;
    if(!isGlobal)
    {
        if(mMemPage->getSize() < (duint)rva)
            return rva;
        auto remainingBytes = mMemPage->getSize() - rva;

        maxByteCountToRead = 16 * (count + 1);
        if(mCodeFoldingManager)
            maxByteCountToRead += mCodeFoldingManager->getFoldedSize(rvaToVa(rva), rvaToVa(rva + maxByteCountToRead));
        maxByteCountToRead = remainingBytes > maxByteCountToRead ? maxByteCountToRead : remainingBytes;
    }
    else
    {
        maxByteCountToRead = 16 * (count + 1);
        if(mCodeFoldingManager)
            maxByteCountToRead += mCodeFoldingManager->getFoldedSize(rvaToVa(rva), rvaToVa(rva + maxByteCountToRead));
    }
    buffer.resize(maxByteCountToRead);

    mMemPage->read(buffer.data(), rva, buffer.size());

    auto newRVA = mDisasm->DisassembleNext((uint8_t*)buffer.data(), rvaToVa(rva), buffer.size(), 0, count);

    newRVA += rva;

    return newRVA;
}

/**
 * @brief       Returns the RVA of count-th instructions before/after (depending on the sign) the given instruction RVA.
 *
 * @param[in]   rva         Instruction RVA
 * @param[in]   count       Instruction count
 *
 * @return      RVA of count-th instructions before/after the given instruction RVA.
 */
duint Disassembly::getInstructionRVA(duint index, dsint count)
{
    duint addr = 0;

    if(count == 0)
        addr = index;
    if(count < 0)
        addr = getPreviousInstructionRVA(index, qAbs(count));
    else if(count > 0)
        addr = getNextInstructionRVA(index, qAbs(count));

    // TODO: fix this sign check
    if(addr < 0)
        addr = 0;
    else if(addr > getRowCount() - 1)
        addr = getRowCount() - 1;

    return addr;
}

/**
 * @brief       Disassembles the instruction at the given RVA.
 *
 * @param[in]   rva     RVA of instruction to disassemble
 *
 * @return      Return the disassembled instruction.
 */
Instruction_t Disassembly::DisassembleAt(duint rva)
{
    if(mMemPage->getSize() < (duint)rva)
        return Instruction_t();

    QByteArray buffer;
    duint base = mMemPage->getBase();
    duint maxByteCountToRead = 16 * 2;

    // Bounding
    auto size = getSize();
    if(!size)
        size = rva + maxByteCountToRead * 2;

    if(mCodeFoldingManager)
        maxByteCountToRead += mCodeFoldingManager->getFoldedSize(rvaToVa(rva), rvaToVa(rva + maxByteCountToRead));

    maxByteCountToRead = maxByteCountToRead > (size - rva) ? (size - rva) : maxByteCountToRead;
    if(!maxByteCountToRead)
        return Instruction_t();

    buffer.resize(maxByteCountToRead);

    if(!mMemPage->read(buffer.data(), rva, buffer.size()))
        return Instruction_t();

    return mDisasm->DisassembleAt((uint8_t*)buffer.data(), buffer.size(), base, rva);
}

/**
 * @brief       Disassembles the instruction count instruction afterc the instruction at the given RVA.
 *              Count can be positive or negative.
 *
 * @param[in]   rva     RVA of reference instruction
 * @param[in]   count   Number of instruction
 *
 * @return      Return the disassembled instruction.
 */
Instruction_t Disassembly::DisassembleAt(duint rva, dsint count)
{
    rva = getNextInstructionRVA(rva, count);
    return DisassembleAt(rva);
}

/************************************************************************************
                                Selection Management
************************************************************************************/
void Disassembly::expandSelectionUpTo(duint to)
{
    if(to < mSelection.firstSelectedIndex)
    {
        mSelection.fromIndex = to;
    }
    else if(to > mSelection.firstSelectedIndex)
    {
        mSelection.toIndex = to;
    }
    else if(to == mSelection.firstSelectedIndex)
    {
        setSingleSelection(to);
    }
}

void Disassembly::setSingleSelection(duint index)
{
    mSelection.firstSelectedIndex = index;
    mSelection.fromIndex = index;
    mSelection.toIndex = getInstructionRVA(mSelection.fromIndex, 1) - 1;
    emit selectionChanged(rvaToVa(index));
}

duint Disassembly::getInitialSelection() const
{
    return mSelection.firstSelectedIndex;
}

duint Disassembly::getSelectionSize() const
{
    return mSelection.toIndex - mSelection.fromIndex + 1;
}

duint Disassembly::getSelectionStart() const
{
    return mSelection.fromIndex;
}

duint Disassembly::getSelectionEnd() const
{
    return mSelection.toIndex;
}

void Disassembly::selectionChangedSlot(duint Va)
{
    if(mXrefInfo.refcount != 0)
    {
        BridgeFree(mXrefInfo.references);
        mXrefInfo.refcount = 0;
    }
    if(DbgIsDebugging())
        DbgXrefGet(Va, &mXrefInfo);
}

void Disassembly::selectNext(bool expand)
{
    auto start = getInstructionRVA(getSelectionStart(), 1) - 1;
    if(expand)
    {
        if(getSelectionEnd() == getInitialSelection() && start != getSelectionEnd()) //decrease down
        {
            auto addr = getInstructionRVA(getSelectionStart(), 1);
            expandSelectionUpTo(addr);
        }
        else //expand down
        {
            auto addr = getSelectionEnd() + 1;
            auto nextRva = getInstructionRVA(addr, 1) - 1;
            expandSelectionUpTo(nextRva);
        }
    }
    else //select next instruction
    {
        auto addr = getSelectionEnd() + 1;
        setSingleSelection(addr);
        auto nextRva = getInstructionRVA(addr, 1) - 1;
        expandSelectionUpTo(nextRva);
    }
}

void Disassembly::selectPrevious(bool expand)
{
    auto start = getInstructionRVA(getSelectionStart(), 1) - 1;
    if(expand)
    {
        if(getSelectionStart() == getInitialSelection() && start != getSelectionEnd()) //decrease up
        {
            auto addr = getInstructionRVA(getSelectionEnd() + 1, -2);
            auto nextRva = getInstructionRVA(addr, 1) - 1;
            expandSelectionUpTo(nextRva);
        }
        else //expand up
        {
            auto addr = getInstructionRVA(start + 1, -2);
            expandSelectionUpTo(addr);
        }
    }
    else
    {
        auto addr = getInstructionRVA(getSelectionStart(), -1);
        setSingleSelection(addr);
        auto nextRva = getInstructionRVA(addr, 1) - 1;
        expandSelectionUpTo(nextRva);
    }
}

bool Disassembly::isSelected(duint base, dsint offset)
{
    auto addr = base;

    if(offset < 0)
        addr = getPreviousInstructionRVA(getTableOffset(), offset);
    else if(offset > 0)
        addr = getNextInstructionRVA(getTableOffset(), offset);

    if(addr >= mSelection.fromIndex && addr <= mSelection.toIndex)
        return true;
    else
        return false;
}

bool Disassembly::isSelected(QList<Instruction_t>* buffer, int index) const
{
    if(buffer->size() > 0 && index >= 0 && index < buffer->size())
    {
        if(buffer->at(index).rva >= mSelection.fromIndex && buffer->at(index).rva <= mSelection.toIndex)
            return true;
        else
            return false;
    }
    else
    {
        return false;
    }
}

duint Disassembly::getSelectedVa() const
{
    // Wrapper around commonly used code:
    // Converts the selected index to a valid virtual address
    return rvaToVa(getInitialSelection());
}

/************************************************************************************
                         Update/Reload/Refresh/Repaint
************************************************************************************/

void Disassembly::prepareDataCount(const QList<duint> & rvas, QList<Instruction_t>* instBuffer)
{
    instBuffer->clear();
    instBuffer->reserve(rvas.count());
    for(int i = 0; i < rvas.count(); i++)
    {
        auto inst = DisassembleAt(rvas.at(i));
        instBuffer->append(inst);
    }
}

void Disassembly::prepareDataRange(duint startRva, duint endRva, const std::function<bool(int, const Instruction_t &)> & disassembled)
{
    auto addrPrev = startRva;
    auto addr = addrPrev;

    int i = 0;
    while(true)
    {
        if(addr > endRva)
            break;
        addrPrev = addr;
        auto inst = DisassembleAt(addr);
        addr = getNextInstructionRVA(addr, 1);
        if(addr == addrPrev)
            break;
        if(!disassembled(i++, inst))
            break;
    }
}

RichTextPainter::List Disassembly::getRichBytes(const Instruction_t & instr, bool isSelected) const
{
    RichTextPainter::List richBytes;
    std::vector<std::pair<size_t, bool>> realBytes;
    formatOpcodeString(instr, richBytes, realBytes);
    dsint cur_addr = rvaToVa(instr.rva);

    if(!richBytes.empty() && richBytes.back().text.endsWith(' '))
        richBytes.back().text.chop(1); //remove trailing space if exists

    auto selectionFromVa = rvaToVa(mSelection.fromIndex);
    auto selectionToVa = rvaToVa(mSelection.toIndex);
    for(size_t i = 0; i < richBytes.size(); i++)
    {
        auto byteIdx = realBytes[i].first;
        auto byteAddr = cur_addr + byteIdx;
        auto isReal = realBytes[i].second;
        RichTextPainter::CustomRichText_t & curByte = richBytes.at(i);
        DBGRELOCATIONINFO relocInfo;
        curByte.underlineColor = mDisassemblyRelocationUnderlineColor;
        if(DbgFunctions()->ModRelocationAtAddr(byteAddr, &relocInfo))
        {
            bool prevInSameReloc = relocInfo.rva < byteAddr - DbgFunctions()->ModBaseFromAddr(byteAddr);
            curByte.underline = isReal;
            curByte.underlineConnectPrev = i > 0 && prevInSameReloc;
        }
        else
        {
            curByte.underline = false;
            curByte.underlineConnectPrev = false;
        }

        DBGPATCHINFO patchInfo;
        if(isReal && DbgFunctions()->PatchGetEx(byteAddr, &patchInfo))
        {
            if((unsigned char)(instr.dump.at((int)byteIdx)) == patchInfo.newbyte)
            {
                curByte.textColor = mModifiedBytesColor;
                curByte.textBackground = mModifiedBytesBackgroundColor;
            }
            else
            {
                curByte.textColor = mRestoredBytesColor;
                curByte.textBackground = mRestoredBytesBackgroundColor;
            }
        }
        else
        {
            curByte.textColor = mBytesColor;
            curByte.textBackground = mBytesBackgroundColor;
        }

        if(curByte.textBackground.alpha() == 0)
        {
            auto byteSelected = byteAddr >= selectionFromVa && byteAddr <= selectionToVa;
            if(isSelected && !byteSelected)
                curByte.textBackground = mBackgroundColor;
            else if(!isSelected && byteSelected)
                curByte.textBackground = mSelectionColor;
        }
    }

    if(mCodeFoldingManager && mCodeFoldingManager->isFolded(cur_addr))
    {
        RichTextPainter::CustomRichText_t curByte;
        curByte.textColor = mBytesColor;
        curByte.textBackground = mBytesBackgroundColor;
        curByte.underlineColor = mDisassemblyRelocationUnderlineColor;
        curByte.underlineWidth = 1;
        curByte.flags = RichTextPainter::FlagAll;
        curByte.underline = false;
        curByte.textColor = mBytesColor;
        curByte.textBackground = mBytesBackgroundColor;
        curByte.text = "...";
        richBytes.push_back(curByte);
    }
    return richBytes;
}

void Disassembly::prepareData()
{
    auto viewableRowsCount = getViewableRowsCount();

    mInstBuffer.clear();
    mInstBuffer.reserve(viewableRowsCount);

    dsint addrPrev = getTableOffset();
    dsint addr = addrPrev;

    mDisasm->UpdateArchitecture();

    for(duint i = 0; i < viewableRowsCount && getRowCount() > 0; i++)
    {
        addrPrev = addr;
        auto inst = DisassembleAt(addr);
        if(inst.length == 0)
            break;
        addr = getNextInstructionRVA(addr, 1);
        if(addr == addrPrev)
            break;
        mInstBuffer.append(inst);
    }

    mSelectedInstruction = DisassembleAt(mSelection.fromIndex);

    setNbrOfLineToPrint(mInstBuffer.size());

    mRichText.resize(getColumnCount());
    for(size_t i = 0; i < mRichText.size(); i++)
    {
        mRichText[i].resize(viewableRowsCount);
        for(size_t j = 0; j < mRichText[i].size(); j++)
        {
            mRichText[i][j].alive = false;
        }
    }
}

void Disassembly::reloadData()
{
    emit selectionChanged(rvaToVa(mSelection.firstSelectedIndex));
    AbstractTableView::reloadData();
}

void Disassembly::paintEvent(QPaintEvent* event)
{
    AbstractTableView::paintEvent(event);

    if(!mAllowPainting)
        return;

    // Delay paint the rich text
    QPainter painter(this->viewport());
    painter.setFont(font());
    int x = -horizontalScrollBar()->value();

    for(int column = 0; column < (int)mRichText.size(); column++)
    {
        int w = getColumnWidth(column);
        int h = getViewableRowsCount() * getRowHeight();

        const bool optimizationEnabled = false;
        if(optimizationEnabled)
        {
            QString columnText;
            columnText.reserve(getColumnWidth(column) * getViewableRowsCount() / getCharWidth());

            QVector<QTextLayout::FormatRange> selections;

            for(int rowOffset = 0; rowOffset < (int)mRichText[column].size(); rowOffset++)
            {
                if(rowOffset > 0)
                    columnText += QChar::LineSeparator;

                const RichTextInfo & info = mRichText[column][rowOffset];
                if(!info.alive)
                    continue;

                for(const RichTextPainter::CustomRichText_t & curRichText : info.richText)
                {
                    if(curRichText.text.isEmpty())
                        continue;

                    if(mFormatCache.empty())
                    {
                        mFormatCache.emplace_back();
                    }

                    QTextLayout::FormatRange range = std::move(mFormatCache.back());
                    mFormatCache.pop_back();
                    range.start = columnText.length();
                    range.length = curRichText.text.length();

                    columnText += curRichText.text;

                    QTextCharFormat & format = range.format;
                    switch(curRichText.flags)
                    {
                    case RichTextPainter::FlagNone: //defaults
                    {
                        format.clearForeground();
                        format.clearBackground();
                    }
                    break;

                    case RichTextPainter::FlagColor: //color only
                    {
                        format.setForeground(curRichText.textColor);
                        format.clearBackground();
                    }
                    break;

                    case RichTextPainter::FlagBackground: //background only
                    {
                        if(curRichText.textBackground.alpha())
                        {
                            format.setBackground(curRichText.textBackground);
                        }
                        else
                        {
                            format.clearBackground();
                        }
                        format.clearForeground();
                    }
                    break;

                    case RichTextPainter::FlagAll: //color+background
                    {
                        if(curRichText.textBackground.alpha())
                        {
                            format.setBackground(curRichText.textBackground);
                        }
                        else
                        {
                            format.clearBackground();
                        }
                        format.setForeground(curRichText.textColor);
                    }
                    break;
                    }

                    if(curRichText.underline)
                    {
                        range.format.setFontUnderline(true);
                        range.format.setUnderlineColor(curRichText.underlineColor);
                    }
                    else
                    {
                        range.format.setFontUnderline(false);
                    }

                    selections.push_back(std::move(range));
                }
            }

            QTextOption textOption;
            textOption.setWrapMode(QTextOption::NoWrap);
            mTextLayout.setTextOption(textOption);

            mTextLayout.setFormats(selections);

            while(!selections.empty())
            {
                mFormatCache.push_back(std::move(selections.back()));
                selections.pop_back();
            }

            mTextLayout.setText(columnText);
            mTextLayout.beginLayout();

            int rowHeight = getRowHeight();
            for(int i = 0, y = 0; ; i++, y += rowHeight)
            {
                QTextLine line = mTextLayout.createLine();
                if(!line.isValid())
                    break;
                const RichTextInfo & info = mRichText[column][i];
                line.setPosition(QPointF(info.xinc, y));
            }

            mTextLayout.endLayout();

            QPixmap pixmap(w - 2, h);
            pixmap.fill(Qt::transparent);

            QPainter clippedPainter;
            clippedPainter.begin(&pixmap);

            mTextLayout.draw(&clippedPainter, QPointF(0, 0));

            clippedPainter.end();

            painter.drawPixmap(x, 0, pixmap);
        }
        else
        {
            for(int rowOffset = 0; rowOffset < (int)mRichText[column].size(); rowOffset++)
            {
                const RichTextInfo & info = mRichText[column][rowOffset];
                if(info.alive)
                    RichTextPainter::paintRichText(&painter, info.x, info.y, info.w, info.h, info.xinc, info.richText, mFontMetrics);
            }
        }

        x += w;
    }
}


/************************************************************************************
                        Public Methods
************************************************************************************/
duint Disassembly::rvaToVa(duint rva) const
{
    return mMemPage->va(rva);
}

void Disassembly::gotoAddress(duint addr)
{
    disassembleAt(addr, true, -1);

    if(mIsMain)
    {
        // Update window title
        DbgCmdExecDirect(QString("guiupdatetitle %1").arg(ToPtrString(addr)));
    }
    GuiUpdateAllViews();
}

void Disassembly::disassembleAt(duint va, bool history, duint newTableOffset)
{
    duint size = 0;
    auto base = DbgMemFindBaseAddr(va, &size);

    unsigned char test = 0;
    if(!base || !size || !DbgMemRead(va, &test, sizeof(test)))
        return;
    duint rva = va - base;

    HistoryData newHistory;

    //VA history
    if(history)
    {
        //truncate everything right from the current VA
        if(mVaHistory.size() && mCurrentVa < mVaHistory.size() - 1) //mCurrentVa is not the last
            mVaHistory.erase(mVaHistory.begin() + mCurrentVa + 1, mVaHistory.end());

        //NOTE: mCurrentVa always points to the last entry of the list

        //add the currently selected address to the history
        duint selectionVA = rvaToVa(getInitialSelection()); //currently selected VA
        duint selectionTableOffset = getTableOffset();
        if(selectionVA && mVaHistory.size() && mVaHistory.last().va != selectionVA) //do not have 2x the same va in a row
        {
            mCurrentVa++;
            newHistory.va = selectionVA;
            newHistory.tableOffset = selectionTableOffset;
            mVaHistory.push_back(newHistory);
        }
    }

    // Set base and size (Useful when memory page changed)
    mMemPage->setAttributes(base, size);
    mDisasm->getEncodeMap()->setMemoryRegion(base);

    if(mRvaDisplayEnabled && mMemPage->getBase() != mRvaDisplayPageBase)
        mRvaDisplayEnabled = false;

    setRowCount(size);

    // Selects disassembled instruction
    setSingleSelection(rva);
    duint instrSize = getInstructionRVA(rva, 1) - rva - 1;
    expandSelectionUpTo(rva + instrSize);

    if(newTableOffset == -1) //nothing specified
    {
        // Update table offset depending on the location of the instruction to disassemble
        if(mInstBuffer.size() > 0 && rva >= mInstBuffer.first().rva && rva < mInstBuffer.last().rva)
        {
            bool isAligned = false;

            // Check if the new RVA is aligned on an instruction from the cache (buffer)
            for(int i = 0; i < mInstBuffer.size(); i++)
            {
                if(mInstBuffer.at(i).rva == rva)
                {
                    isAligned = true;
                    break;
                }
            }

            if(isAligned)
            {
                updateViewport();
            }
            else
            {
                setTableOffset(rva);
            }
        }
        else if(mInstBuffer.size() > 0 && rva == (dsint)mInstBuffer.last().rva)
        {
            setTableOffset(mInstBuffer.first().rva + mInstBuffer.first().length);
        }
        else
        {
            setTableOffset(rva);
        }

        if(history)
        {
            //new disassembled address
            newHistory.va = va;
            newHistory.tableOffset = getTableOffset();
            if(mVaHistory.size())
            {
                if(mVaHistory.last().va != va) //not 2x the same va in history
                {
                    if(mVaHistory.size() >= 1024) //max 1024 in the history
                    {
                        mCurrentVa--;
                        mVaHistory.erase(mVaHistory.begin()); //remove the oldest element
                    }
                    mCurrentVa++;
                    mVaHistory.push_back(newHistory); //add a va to the history
                }
            }
            else //the list is empty
                mVaHistory.push_back(newHistory);
        }
    }
    else //specified new table offset
        setTableOffset(newTableOffset);

    /*
    //print history
    if(history)
    {
        QString strList = "";
        for(int i=0; i<mVaHistory.size(); i++)
            strList += QString().sprintf("[%d]:%p,%p\n", i, mVaHistory.at(i).va, mVaHistory.at(i).tableOffset);
        MessageBoxA(GuiGetWindowHandle(), strList.toUtf8().constData(), QString().sprintf("mCurrentVa=%d", mCurrentVa).toUtf8().constData(), MB_ICONINFORMATION);
    }
    */
}

QList<Instruction_t>* Disassembly::instructionsBuffer()
{
    return &mInstBuffer;
}

void Disassembly::disassembleAtSlot(duint va, duint cip)
{
    if(cip == 0)
        cip = mCipVa;

    if(mCodeFoldingManager)
    {
        mCodeFoldingManager->expandFoldSegment(va);
        mCodeFoldingManager->expandFoldSegment(cip);
    }
    mCipVa = cip;
    if(mIsMain || !mMemPage->getBase())
        disassembleAt(va, true, -1);
}

void Disassembly::disassembleClear()
{
    mHighlightingMode = false;
    mHighlightToken = ZydisTokenizer::SingleToken();
    historyClear();
    mMemPage->setAttributes(0, 0);
    mDisasm->getEncodeMap()->setMemoryRegion(0);
    setRowCount(0);
    setTableOffset(0);
    mInstBuffer.clear();
    reloadData();
}

void Disassembly::debugStateChangedSlot(DBGSTATE state)
{
    switch(state)
    {
    case stopped:
        disassembleClear();
        break;
    default:
        break;
    }
}

const duint Disassembly::getBase() const
{
    return mMemPage->getBase();
}

duint Disassembly::getSize() const
{
    return mMemPage->getSize();
}

duint Disassembly::getTableOffsetRva() const
{
    return mInstBuffer.size() ? mInstBuffer.at(0).rva : 0;
}

void Disassembly::historyClear()
{
    mVaHistory.clear(); //clear history for new targets
    mCurrentVa = 0;
}

void Disassembly::historyPrevious()
{
    if(!historyHasPrevious())
        return;
    mCurrentVa--;
    dsint va = mVaHistory.at(mCurrentVa).va;
    if(mCodeFoldingManager && mCodeFoldingManager->isFolded(va))
        mCodeFoldingManager->expandFoldSegment(va);
    disassembleAt(va, false, mVaHistory.at(mCurrentVa).tableOffset);

    if(mIsMain)
    {
        // Update window title
        DbgCmdExecDirect(QString("guiupdatetitle %1").arg(ToPtrString(va)));
        GuiUpdateAllViews();
    }
}

void Disassembly::historyNext()
{
    if(!historyHasNext())
        return;
    mCurrentVa++;
    dsint va = mVaHistory.at(mCurrentVa).va;
    if(mCodeFoldingManager && mCodeFoldingManager->isFolded(va))
        mCodeFoldingManager->expandFoldSegment(va);
    disassembleAt(va, false, mVaHistory.at(mCurrentVa).tableOffset);

    if(mIsMain)
    {
        // Update window title
        DbgCmdExecDirect(QString("guiupdatetitle %1").arg(ToPtrString(va)));
        GuiUpdateAllViews();
    }
}

bool Disassembly::historyHasPrevious() const
{
    if(!mCurrentVa || !mVaHistory.size()) //we are at the earliest history entry
        return false;
    return true;
}

bool Disassembly::historyHasNext() const
{
    int size = mVaHistory.size();
    if(!size || mCurrentVa >= mVaHistory.size() - 1) //we are at the newest history entry
        return false;
    return true;
}

QString Disassembly::getAddrText(duint cur_addr, QString & label, bool getLabel)
{
    QString addrText = "";
    if(mRvaDisplayEnabled) //RVA display
    {
        dsint rva = cur_addr - mRvaDisplayBase;
        if(rva == 0)
        {
            if(mArchitecture->addr64())
                addrText = "$ ==>            ";
            else
                addrText = "$ ==>    ";
        }
        else if(rva > 0)
        {
            if(mArchitecture->addr64())
                addrText = "$+" + QString("%1").arg(rva, -15, 16, QChar(' ')).toUpper();
            else
                addrText = "$+" + QString("%1").arg(rva, -7, 16, QChar(' ')).toUpper();
        }
        else if(rva < 0)
        {
            if(mArchitecture->addr64())
                addrText = "$-" + QString("%1").arg(-rva, -15, 16, QChar(' ')).toUpper();
            else
                addrText = "$-" + QString("%1").arg(-rva, -7, 16, QChar(' ')).toUpper();
        }
    }
    addrText += ToPtrString(cur_addr);
    char label_[MAX_LABEL_SIZE] = "";
    if(getLabel && DbgGetLabelAt(cur_addr, SEG_DEFAULT, label_)) //has label
    {
        char module[MAX_MODULE_SIZE] = "";
        if(DbgGetModuleAt(cur_addr, module) && !QString(label_).startsWith("JMP.&") && !mNoCurrentModuleText)
            addrText += " <" + QString(module) + "." + QString(label_) + ">";
        else
            addrText += " <" + QString(label_) + ">";
    }
    else
        *label_ = 0;
    label = label_;
    return addrText;
}

/**
 * @brief Set the code folding manager for the disassembly view
 * @param CodeFoldingManager The pointer to the code folding manager.
 */
void Disassembly::setCodeFoldingManager(CodeFoldingHelper* CodeFoldingManager)
{
    mCodeFoldingManager = CodeFoldingManager;
    mDisasm->setCodeFoldingManager(CodeFoldingManager);
}

/**
 * @brief   Unfolds specified rva.
 * @param rva the address.
 */
void Disassembly::unfold(duint rva)
{
    if(mCodeFoldingManager)
    {
        mCodeFoldingManager->expandFoldSegment(rvaToVa(rva));
        viewport()->update();
    }
}

bool Disassembly::hightlightToken(const ZydisTokenizer::SingleToken & token)
{
    mHighlightToken = token;
    mHighlightingMode = false;
    return true;
}

bool Disassembly::isHighlightMode() const
{
    return mHighlightingMode;
}
