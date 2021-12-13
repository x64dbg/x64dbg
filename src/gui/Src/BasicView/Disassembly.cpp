#include "Disassembly.h"
#include "Configuration.h"
#include "CodeFolding.h"
#include "EncodeMap.h"
#include "Bridge.h"
#include "CachedFontMetrics.h"
#include "QBeaEngine.h"
#include "MemoryPage.h"

Disassembly::Disassembly(QWidget* parent, bool isMain)
    : AbstractTableView(parent),
      mIsMain(isMain)
{
    mMemPage = new MemoryPage(0, 0);

    mInstBuffer.clear();
    setDrawDebugOnly(true);

    historyClear();

    memset(&mSelection, 0, sizeof(SelectionData));

    mHighlightToken.text = "";
    mHighlightingMode = false;
    mShowMnemonicBrief = false;

    int maxModuleSize = (int)ConfigUint("Disassembler", "MaxModuleSize");
    Config()->writeUints();

    mDisasm = new QBeaEngine(maxModuleSize);
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
    addColumnAt(1000, tr("Comments"), false); //comments

    setShowHeader(false); //hide header

    mBackgroundColor = ConfigColor("DisassemblyBackgroundColor");

    mXrefInfo.refcount = 0;

    // Slots
    connect(Bridge::getBridge(), SIGNAL(updateDisassembly()), this, SLOT(reloadData()));
    connect(Bridge::getBridge(), SIGNAL(dbgStateChanged(DBGSTATE)), this, SLOT(debugStateChangedSlot(DBGSTATE)));
    connect(this, SIGNAL(selectionChanged(dsint)), this, SLOT(selectionChangedSlot(dsint)));
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
    setDisassemblyPopupEnabled(!Config()->getBool("Disassembler", "NoBranchDisasmPreview"));
}

void Disassembly::tokenizerConfigUpdatedSlot()
{
    mDisasm->UpdateConfig();
    mPermanentHighlightingMode = ConfigBool("Disassembler", "PermanentHighlightingMode");
    mNoCurrentModuleText = ConfigBool("Disassembler", "NoCurrentModuleText");
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
QString Disassembly::paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h)
{
    Q_UNUSED(rowBase);

    if(mHighlightingMode)
    {
        QPen pen(Qt::red);
        pen.setWidth(2);
        painter->setPen(pen);
        QRect rect = viewport()->rect();
        rect.adjust(1, 1, -1, -1);
        painter->drawRect(rect);
    }
    dsint wRVA = mInstBuffer.at(rowOffset).rva;
    bool wIsSelected = isSelected(&mInstBuffer, rowOffset);
    dsint cur_addr = rvaToVa(mInstBuffer.at(rowOffset).rva);
    auto traceCount = DbgFunctions()->GetTraceRecordHitCount(cur_addr);

    // Highlight if selected
    if(wIsSelected && traceCount)
        painter->fillRect(QRect(x, y, w, h), QBrush(mTracedSelectedAddressBackgroundColor));
    else if(wIsSelected)
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
        richText.textBackground = mBackgroundColor; // TODO: this has to extend the whole column width
        richText.flags = RichTextPainter::FlagAll;

        char label[MAX_LABEL_SIZE] = "";
        QString addrText = getAddrText(cur_addr, label);
        richText.text = addrText;
        BPXTYPE bpxtype = DbgGetBpxTypeAt(cur_addr);
        bool isbookmark = DbgGetBookmarkAt(cur_addr);
        if(rvaToVa(mInstBuffer.at(rowOffset).rva) == mCipVa && !Bridge::getBridge()->mIsRunning && DbgMemFindBaseAddr(DbgValFromString("cip"), nullptr)) //cip + not running + valid cip
        {
            richText.textBackground = mCipBackgroundColor;
            //painter->fillRect(QRect(x, y, w, h), QBrush(mCipBackgroundColor));
            if(!isbookmark) //no bookmark
            {
                if(bpxtype & bp_normal) //normal breakpoint
                {
                    QColor & bpColor = mBreakpointBackgroundColor;
                    if(!bpColor.alpha()) //we don't want transparent text
                        bpColor = mBreakpointColor;
                    if(bpColor == mCipBackgroundColor)
                        bpColor = mCipColor;
                    richText.textColor = bpColor;
                    //painter->setPen(bpColor);
                }
                else if(bpxtype & bp_hardware) //hardware breakpoint only
                {
                    QColor hwbpColor = mHardwareBreakpointBackgroundColor;
                    if(!hwbpColor.alpha()) //we don't want transparent text
                        hwbpColor = mHardwareBreakpointColor;
                    if(hwbpColor == mCipBackgroundColor)
                        hwbpColor = mCipColor;
                    richText.textColor = hwbpColor;
                    //painter->setPen(hwbpColor);
                }
                else //no breakpoint
                {
                    richText.textColor = mCipColor;
                    //painter->setPen(mCipColor);
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
                //painter->setPen(bookmarkColor);
            }
        }
        else //non-cip address
        {
            if(!isbookmark) //no bookmark
            {
                if(*label) //label
                {
                    if(bpxtype == bp_none) //label only : fill label background
                    {
                        //painter->setPen(mLabelColor); //red -> address + label text
                        //painter->fillRect(QRect(x, y, w, h), QBrush(mLabelBackgroundColor)); //fill label background
                        richText.textColor = mLabelColor;
                        richText.textBackground = mLabelBackgroundColor;
                    }
                    else //label + breakpoint
                    {
                        if(bpxtype & bp_normal) //label + normal breakpoint
                        {
                            richText.textColor = mBreakpointColor;
                            richText.textBackground = mBreakpointBackgroundColor;
                            //painter->setPen(mBreakpointColor);
                            //painter->fillRect(QRect(x, y, w, h), QBrush(mBreakpointBackgroundColor)); //fill red
                        }
                        else if(bpxtype & bp_hardware) //label + hardware breakpoint only
                        {
                            richText.textColor = mHardwareBreakpointColor;
                            richText.textBackground = mHardwareBreakpointBackgroundColor;
                            //painter->setPen(mHardwareBreakpointColor);
                            //painter->fillRect(QRect(x, y, w, h), QBrush(mHardwareBreakpointBackgroundColor)); //fill ?
                        }
                        else //other cases -> do as normal
                        {
                            richText.textColor = mLabelColor;
                            richText.textBackground = mLabelBackgroundColor;
                            //painter->setPen(mLabelColor); //red -> address + label text
                            //painter->fillRect(QRect(x, y, w, h), QBrush(mLabelBackgroundColor)); //fill label background
                        }
                    }
                }
                else //no label
                {
                    if(bpxtype == bp_none) //no label, no breakpoint
                    {
                        //QColor background;
                        if(wIsSelected)
                        {
                            //background = mSelectedAddressBackgroundColor;
                            //painter->setPen(mSelectedAddressColor); //black address (DisassemblySelectedAddressColor)
                            richText.textColor = mSelectedAddressColor;
                            richText.textBackground = mSelectedAddressBackgroundColor;
                        }
                        else
                        {
                            //background = mAddressBackgroundColor;
                            //painter->setPen(mAddressColor); //DisassemblyAddressColor
                            richText.textColor = mAddressColor;
                            richText.textBackground = mAddressBackgroundColor;
                        }
                        /*if(background.alpha())
                            painter->fillRect(QRect(x, y, w, h), QBrush(background)); //fill background*/
                    }
                    else //breakpoint only
                    {
                        if(bpxtype & bp_normal) //normal breakpoint
                        {
                            richText.textColor = mBreakpointColor;
                            richText.textBackground = mBreakpointBackgroundColor;
                            //painter->setPen(mBreakpointColor);
                            //painter->fillRect(QRect(x, y, w, h), QBrush(mBreakpointBackgroundColor)); //fill red
                        }
                        else if(bpxtype & bp_hardware) //hardware breakpoint only
                        {
                            richText.textColor = mHardwareBreakpointColor;
                            richText.textBackground = mHardwareBreakpointBackgroundColor;
                            //painter->setPen(mHardwareBreakpointColor);
                            //painter->fillRect(QRect(x, y, w, h), QBrush(mHardwareBreakpointBackgroundColor)); //fill red
                        }
                        else //other cases (memory breakpoint in disassembly) -> do as normal
                        {
                            //QColor background;
                            if(wIsSelected)
                            {
                                richText.textColor = mSelectedAddressColor;
                                richText.textBackground = mSelectedAddressBackgroundColor;
                                //background = mSelectedAddressBackgroundColor;
                                //painter->setPen(mSelectedAddressColor); //black address (DisassemblySelectedAddressColor)
                            }
                            else
                            {
                                richText.textColor = mAddressColor;
                                richText.textBackground = mAddressBackgroundColor;
                                //background = mAddressBackgroundColor;
                                //painter->setPen(mAddressColor);
                            }
                            /*if(background.alpha())
                                painter->fillRect(QRect(x, y, w, h), QBrush(background)); //fill background*/
                        }
                    }
                }
            }
            else //bookmark
            {
                if(*label) //label + bookmark
                {
                    if(bpxtype == bp_none) //label + bookmark
                    {
                        richText.textColor = mLabelColor;
                        richText.textBackground = mBookmarkBackgroundColor;
                        //painter->setPen(mLabelColor); //red -> address + label text
                        //painter->fillRect(QRect(x, y, w, h), QBrush(mBookmarkBackgroundColor)); //fill label background
                    }
                    else //label + breakpoint + bookmark
                    {
                        QColor color = mBookmarkBackgroundColor;
                        if(!color.alpha()) //we don't want transparent text
                            color = mAddressColor;
                        //painter->setPen(color);
                        richText.textColor = color;
                        if(bpxtype & bp_normal) //label + bookmark + normal breakpoint
                        {
                            richText.textBackground = mBreakpointBackgroundColor;
                            //painter->fillRect(QRect(x, y, w, h), QBrush(mBreakpointBackgroundColor)); //fill red
                        }
                        else if(bpxtype & bp_hardware) //label + bookmark + hardware breakpoint only
                        {
                            richText.textBackground = mHardwareBreakpointBackgroundColor;
                            //painter->fillRect(QRect(x, y, w, h), QBrush(mHardwareBreakpointBackgroundColor)); //fill ?
                        }
                    }
                }
                else //bookmark, no label
                {
                    if(bpxtype == bp_none) //bookmark only
                    {
                        richText.textColor = mBookmarkColor;
                        richText.textBackground = mBookmarkBackgroundColor;
                        //painter->setPen(mBookmarkColor); //black address
                        //painter->fillRect(QRect(x, y, w, h), QBrush(mBookmarkBackgroundColor)); //fill bookmark color
                    }
                    else //bookmark + breakpoint
                    {
                        QColor color = mBookmarkBackgroundColor;
                        if(!color.alpha()) //we don't want transparent text
                            color = mAddressColor;
                        //painter->setPen(color);
                        richText.textColor = color;
                        if(bpxtype & bp_normal) //bookmark + normal breakpoint
                        {
                            richText.textBackground = mBreakpointBackgroundColor;
                            //painter->fillRect(QRect(x, y, w, h), QBrush(mBreakpointBackgroundColor)); //fill red
                        }
                        else if(bpxtype & bp_hardware) //bookmark + hardware breakpoint only
                        {
                            richText.textBackground = mHardwareBreakpointBackgroundColor;
                            //painter->fillRect(QRect(x, y, w, h), QBrush(mHardwareBreakpointBackgroundColor)); //fill red
                        }
                        else //other cases (bookmark + memory breakpoint in disassembly) -> do as normal
                        {
                            richText.textColor = mBookmarkColor;
                            richText.textBackground = mBookmarkBackgroundColor;
                            //painter->setPen(mBookmarkColor); //black address
                            //painter->fillRect(QRect(x, y, w, h), QBrush(mBookmarkBackgroundColor)); //fill bookmark color
                        }
                    }
                }
            }
        }
        RichTextPainter::List list;
        list.emplace_back(std::move(richText));
        paintRichText(painter, x, y, w, h, 4, list, rowOffset, col);
        //painter->drawText(QRect(x + 4, y, w - 4, h), Qt::AlignVCenter | Qt::AlignLeft, addrText);
    }
    break;

    case ColBytes: //draw bytes
    {
        const Instruction_t & instr = mInstBuffer.at(rowOffset);
        //draw functions
        Function_t funcType;
        FUNCTYPE funcFirst = DbgGetFunctionTypeAt(cur_addr);
        FUNCTYPE funcLast = DbgGetFunctionTypeAt(cur_addr + instr.length - 1);
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

        XREFTYPE refType = DbgGetXrefTypeAt(cur_addr);
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
        Instruction_t::BranchType branchType = mInstBuffer.at(rowOffset).branchType;
        int jumpsize = paintJumpsGraphic(painter, x + funcsize, y - 1, wRVA, branchType != Instruction_t::None && branchType != Instruction_t::Call); //jump line

        //draw bytes
        auto richBytes = getRichBytes(instr, wIsSelected);
        paintRichText(painter, x, y, getColumnWidth(col), getRowHeight(), jumpsize + funcsize, richBytes, rowOffset, col);
    }
    break;

    case ColDisassembly: //draw disassembly (with colours needed)
    {
        int loopsize = 0;
        int depth = 0;

        while(1) //paint all loop depths
        {
            LOOPTYPE loopFirst = DbgGetLoopTypeAt(cur_addr, depth);
            LOOPTYPE loopLast = DbgGetLoopTypeAt(cur_addr + mInstBuffer.at(rowOffset).length - 1, depth);
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
        int xinc = 4;
        paintRichText(painter, x + loopsize, y, getColumnWidth(col) - loopsize, getRowHeight(), xinc, richText, rowOffset, col);
        token.x = x + loopsize + xinc;
    }
    break;

    case ColComment: //draw comments
    {
        //draw arguments
        Function_t funcType;
        ARGTYPE argFirst = DbgGetArgTypeAt(cur_addr);
        ARGTYPE argLast = DbgGetArgTypeAt(cur_addr + mInstBuffer.at(rowOffset).length - 1);
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
        int argsize = funcType == Function_none ? 3 : paintFunctionGraphic(painter, x, y, funcType, false);

        RichTextPainter::CustomRichText_t richComment;
        richComment.underline = false;
        richComment.textColor = mTextColor;
        richComment.textBackground = mBackgroundColor;
        richComment.flags = RichTextPainter::FlagAll;

        QString comment;
        bool autoComment = false;
        char label[MAX_LABEL_SIZE] = "";
        if(GetCommentFormat(cur_addr, comment, &autoComment))
        {
            //QColor backgroundColor;
            if(autoComment)
            {
                richComment.textColor = mAutoCommentColor;
                richComment.textBackground = mAutoCommentBackgroundColor;
                //painter->setPen(mAutoCommentColor);
                //backgroundColor = mAutoCommentBackgroundColor;
            }
            else //user comment
            {
                richComment.textColor = mCommentColor;
                richComment.textBackground = mCommentBackgroundColor;
                //painter->setPen(mCommentColor);
                //backgroundColor = mCommentBackgroundColor;
            }

            richComment.text = std::move(comment);

            /*int width = mFontMetrics->width(comment);
            if(width > w)
                width = w;
            if(width)
                painter->fillRect(QRect(x + argsize, y, width, h), QBrush(backgroundColor)); //fill comment color
            painter->drawText(QRect(x + argsize, y, width, h), Qt::AlignVCenter | Qt::AlignLeft, comment);*/
            //argsize += width + 3;
        }
        else if(DbgGetLabelAt(cur_addr, SEG_DEFAULT, label)) // label but no comment
        {
            //QString labelText(label);
            //QColor backgroundColor;
            //painter->setPen(mLabelColor);
            //backgroundColor = mLabelBackgroundColor;

            richComment.textColor = mLabelColor;
            richComment.textBackground = mLabelBackgroundColor;
            richComment.text = label;

            /*int width = mFontMetrics->width(labelText);
            if(width > w)
                width = w;
            if(width)
                painter->fillRect(QRect(x + argsize, y, width, h), QBrush(backgroundColor)); //fill comment color
            painter->drawText(QRect(x + argsize, y, width, h), Qt::AlignVCenter | Qt::AlignLeft, labelText);*/
            //argsize += width + 3;
        }

        RichTextPainter::List richText;
        richText.emplace_back(std::move(richComment));

        if(mShowMnemonicBrief)
        {
            RichTextPainter::CustomRichText_t richBrief;
            richBrief.underline = false;
            richBrief.textColor = mMnemonicBriefColor;
            richBrief.textBackground = mMnemonicBriefBackgroundColor;
            richBrief.flags = RichTextPainter::FlagAll;

            char brief[MAX_STRING_SIZE] = "";
            QString mnem;
            for(const ZydisTokenizer::SingleToken & token : mInstBuffer.at(rowOffset).tokens.tokens)
            {
                if(token.type != ZydisTokenizer::TokenType::Space && token.type != ZydisTokenizer::TokenType::Prefix)
                {
                    mnem = token.text;
                    break;
                }
            }
            if(mnem.isEmpty())
                mnem = mInstBuffer.at(rowOffset).instStr;

            int index = mnem.indexOf(' ');
            if(index != -1)
                mnem.truncate(index);
            DbgFunctions()->GetMnemonicBrief(mnem.toUtf8().constData(), MAX_STRING_SIZE, brief);

            //painter->setPen(mMnemonicBriefColor);

            QString mnemBrief = brief;
            if(mnemBrief.length())
            {
                /*int width = mFontMetrics->width(mnemBrief);
                if(width > w)
                    width = w;
                if(width)
                    painter->fillRect(QRect(x + argsize, y, width, h), QBrush(mMnemonicBriefBackgroundColor)); //mnemonic brief background color
                painter->drawText(QRect(x + argsize, y, width, h), Qt::AlignVCenter | Qt::AlignLeft, mnemBrief);*/

                RichTextPainter::CustomRichText_t space;
                space.underline = false;
                space.flags = RichTextPainter::FlagNone;
                space.text = " ";
                richText.emplace_back(std::move(space));

                richBrief.text = std::move(mnemBrief);

                richText.emplace_back(std::move(richBrief));
            }
        }

        paintRichText(painter, x, y, w, h, argsize, richText, rowOffset, col);
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

    bool wAccept = true;
    int y = event->y();

    if(mGuiState == Disassembly::MultiRowsSelectionState)
    {
        //qDebug() << "State = MultiRowsSelectionState";

        if((transY(y) >= 0) && (transY(y) <= this->getTableHeight()))
        {
            int wI = getIndexOffsetFromY(transY(y));

            if(mMemPage->getSize() > 0)
            {
                // Bound
                wI = wI >= mInstBuffer.size() ? mInstBuffer.size() - 1 : wI;
                wI = wI < 0 ? 0 : wI;

                if(wI >= mInstBuffer.size())
                    return;
                dsint wRowIndex = mInstBuffer.at(wI).rva;
                dsint wInstrSize = getInstructionRVA(wRowIndex, 1) - wRowIndex - 1;

                if(wRowIndex < getRowCount())
                {
                    setSingleSelection(getInitialSelection());
                    expandSelectionUpTo(getInstructionRVA(getInitialSelection(), 1) - 1);
                    if(wRowIndex > getInitialSelection()) //select down
                        expandSelectionUpTo(wRowIndex + wInstrSize);
                    else
                        expandSelectionUpTo(wRowIndex);

                    emit selectionExpanded();
                    updateViewport();

                    wAccept = false;
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

    if(wAccept == true)
        AbstractTableView::mouseMoveEvent(event);
}

duint Disassembly::getDisassemblyPopupAddress(int mousex, int mousey)
{
    if(mHighlightingMode)
        return 0; //Don't show this in highlight mode
    if(getColumnIndexFromX(mousex) != 2)
        return 0; //Disassembly popup for other column is undefined
    int rowOffset = getIndexOffsetFromY(transY(mousey));
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
    bool wAccept = false;

    if(mHighlightingMode || mPermanentHighlightingMode)
    {
        if(getColumnIndexFromX(event->x()) == 2) //click in instruction column
        {
            int rowOffset = getIndexOffsetFromY(transY(event->y()));
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
                dsint wIndex = getIndexOffsetFromY(transY(event->y()));

                if(mInstBuffer.size() > wIndex && wIndex >= 0)
                {
                    dsint wRowIndex = mInstBuffer.at(wIndex).rva;
                    dsint wInstrSize = mInstBuffer.at(wIndex).length - 1;
                    if(!(event->modifiers() & Qt::ShiftModifier)) //SHIFT pressed
                        setSingleSelection(wRowIndex);
                    if(getSelectionStart() > wRowIndex) //select up
                    {
                        setSingleSelection(getInitialSelection());
                        expandSelectionUpTo(getInstructionRVA(getInitialSelection(), 1) - 1);
                        expandSelectionUpTo(wRowIndex);
                    }
                    else //select down
                    {
                        setSingleSelection(getInitialSelection());
                        expandSelectionUpTo(wRowIndex + wInstrSize);
                    }

                    mGuiState = Disassembly::MultiRowsSelectionState;

                    updateViewport();

                    wAccept = true;
                }
            }
        }
    }

    if(wAccept == false)
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
    bool wAccept = true;

    if((event->buttons() & Qt::LeftButton) == 0)
    {
        if(mGuiState == Disassembly::MultiRowsSelectionState)
        {
            mGuiState = Disassembly::NoState;

            updateViewport();

            wAccept = false;
        }
    }
    if((event->button() & Qt::BackButton) != 0)
    {
        wAccept = true;
        historyPrevious();
    }
    else if((event->button() & Qt::ForwardButton) != 0)
    {
        wAccept = true;
        historyNext();
    }

    if(wAccept == true)
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
        ShowDisassemblyPopup(0, 0, 0);

        if(key == Qt::Key_Left)
        {
            setTableOffset(getTableOffset() - 1);
        }
        else if(key == Qt::Key_Right)
        {
            setTableOffset(getTableOffset() + 1);
        }

        updateViewport();
    }
    else if(key == Qt::Key_Up || key == Qt::Key_Down)
    {
        ShowDisassemblyPopup(0, 0, 0);

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
            setTableOffset(getInstructionRVA(modifiedSelection, -getNbrOfLineToPrint() + 2));
        }

        updateViewport();
    }
    else if(key == Qt::Key_Return || key == Qt::Key_Enter)
    {
        ShowDisassemblyPopup(0, 0, 0);
        // Follow branch instruction
        duint dest = DbgGetBranchDestination(rvaToVa(getInitialSelection()));
        if(DbgMemIsValidReadPtr(dest))
        {
            gotoAddress(dest);
            return;
        }
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
dsint Disassembly::sliderMovedHook(int type, dsint value, dsint delta)
{
    ShowDisassemblyPopup(0, 0, 0);

    // QAbstractSlider::SliderNoAction is used to disassembe at a specific address
    if(type == QAbstractSlider::SliderNoAction)
        return value + delta;

    // If it's a slider action, disassemble one instruction back and one instruction next in order to be aligned on a real instruction
    if(type == QAbstractSlider::SliderMove)
    {
        dsint wNewValue = 0;

        if(value + delta > 0)
        {
            wNewValue = getInstructionRVA(value + delta, -1);
            wNewValue = getInstructionRVA(wNewValue, 1);
        }

        return wNewValue;
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
int Disassembly::paintJumpsGraphic(QPainter* painter, int x, int y, dsint addr, bool isjmp)
{
    dsint selHeadRVA = mSelection.fromIndex;
    dsint rva = addr;
    duint curVa = rvaToVa(addr);
    duint selVa = rvaToVa(mSelection.firstSelectedIndex);
    Instruction_t instruction = DisassembleAt(selHeadRVA);
    auto branchType = instruction.branchType;

    bool showXref = false;

    GraphicDump wPict = GD_Nothing;

    if(branchType != Instruction_t::None && branchType != Instruction_t::Call)
    {
        dsint base = mMemPage->getBase();
        dsint destVA = DbgGetBranchDestination(rvaToVa(selHeadRVA));

        if(destVA >= base && destVA < base + (dsint)mMemPage->getSize())
        {
            destVA -= base;

            if(destVA < selHeadRVA)
            {
                if(rva == destVA)
                    wPict = GD_HeadFromBottom;
                else if(rva > destVA && rva < selHeadRVA)
                    wPict = GD_Vert;
                else if(rva == selHeadRVA)
                    wPict = GD_FootToTop;
            }
            else if(destVA > selHeadRVA)
            {
                if(rva == selHeadRVA)
                    wPict = GD_FootToBottom;
                else if(rva > selHeadRVA && rva < destVA)
                    wPict = GD_Vert;
                else if(rva == destVA)
                    wPict = GD_HeadFromTop;
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
                wPict = GD_VertHori;
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
                    wPict = GD_HeadFromTop;
                }
                else if(min == selVa)
                {
                    wPict = GD_HeadFromBottom;
                }
                else if(max > selVa && min < selVa)
                {
                    wPict = GD_HeadFromBoth;
                }

            }
            else if(curVa < selVa && curVa == min)
            {
                wPict =  GD_FootToBottom;
            }
            else if(curVa > selVa && curVa == max)
            {
                wPict = GD_FootToTop;
            }
            if(wPict == GD_Nothing && curVa > min && curVa < max)
                wPict = GD_Vert;
        }
    }

    GraphicJumpDirection curInstDir = GJD_Nothing;

    if(isjmp)
    {
        duint curInstDestination = DbgGetBranchDestination(curVa);
        if(curInstDestination == 0 || curVa == curInstDestination)
        {
            curInstDir = GJD_Nothing;
        }
        else if(curInstDestination < curVa)
        {
            curInstDir = GJD_Up;
        }
        else
        {
            curInstDir = GJD_Down;
        }
    }

    int halfRow = getRowHeight() / 2 + 1;

    painter->setPen(mConditionalTruePen);
    if(curInstDir == GJD_Up)
    {
        QPoint wPoints[] =
        {
            QPoint(x, y + halfRow + 1),
            QPoint(x + 2, y + halfRow - 1),
            QPoint(x + 4, y + halfRow + 1),
        };

        painter->drawPolyline(wPoints, 3);
    }
    else if(curInstDir == GJD_Down)
    {
        QPoint wPoints[] =
        {
            QPoint(x, y + halfRow - 1),
            QPoint(x + 2, y + halfRow + 1),
            QPoint(x + 4, y + halfRow - 1),
        };

        painter->drawPolyline(wPoints, 3);
    }

    x += 8;

    if(showXref)
    {
        painter->setPen(mUnconditionalPen);
    }
    else
    {
        bool bIsExecute = DbgIsJumpGoingToExecute(rvaToVa(instruction.rva));


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



    if(wPict == GD_Vert)
    {
        painter->drawLine(x, y, x, y + getRowHeight());
    }
    else if(wPict == GD_FootToBottom)
    {
        painter->drawLine(x, y + halfRow, x + 5, y + halfRow);
        painter->drawLine(x, y + halfRow, x, y + getRowHeight());
    }
    else if(wPict == GD_FootToTop)
    {
        painter->drawLine(x, y + halfRow, x + 5, y + halfRow);
        painter->drawLine(x, y, x, y + halfRow);
    }
    else if(wPict == GD_HeadFromBottom)
    {
        QPoint wPoints[] =
        {
            QPoint(x + 3, y + halfRow - 2),
            QPoint(x + 5, y + halfRow),
            QPoint(x + 3, y + halfRow + 2),
        };

        painter->drawLine(x, y + halfRow, x + 5, y + halfRow);
        painter->drawLine(x, y + halfRow, x, y + getRowHeight());
        painter->drawPolyline(wPoints, 3);
    }
    else if(wPict == GD_HeadFromTop)
    {
        QPoint wPoints[] =
        {
            QPoint(x + 3, y + halfRow - 2),
            QPoint(x + 5, y + halfRow),
            QPoint(x + 3, y + halfRow + 2),
        };

        painter->drawLine(x, y + halfRow, x + 5, y + halfRow);
        painter->drawLine(x, y, x, y + halfRow);
        painter->drawPolyline(wPoints, 3);
    }
    else if(wPict == GD_HeadFromBoth)
    {
        QPoint wPoints[] =
        {
            QPoint(x + 3, y + halfRow - 2),
            QPoint(x + 5, y + halfRow),
            QPoint(x + 3, y + halfRow + 2),
        };

        painter->drawLine(x, y + halfRow, x + 5, y + halfRow);
        painter->drawLine(x, y, x, y + getRowHeight());
        painter->drawPolyline(wPoints, 3);
    }
    else if(wPict == GD_VertHori)
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

void Disassembly::paintRichText(QPainter* painter, int x, int y, int w, int h, int xinc, const RichTextPainter::List & richText, int rowOffset, int column)
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

void Disassembly::paintRichText(QPainter* painter, int x, int y, int w, int h, int xinc, const RichTextPainter::List && richText, int rowOffset, int column)
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
dsint Disassembly::getPreviousInstructionRVA(dsint rva, duint count)
{
    QByteArray wBuffer;
    dsint wBottomByteRealRVA;
    dsint wVirtualRVA;
    dsint wMaxByteCountToRead;

    wBottomByteRealRVA = (dsint)rva - 16 * (count + 3);
    if(mCodeFoldingManager)
    {
        if(mCodeFoldingManager->isFolded(rvaToVa(wBottomByteRealRVA)))
        {
            wBottomByteRealRVA = mCodeFoldingManager->getFoldBegin(wBottomByteRealRVA) - mMemPage->getBase() - 16 * (count + 3);
        }
    }
    wBottomByteRealRVA = wBottomByteRealRVA < 0 ? 0 : wBottomByteRealRVA;

    wVirtualRVA = (dsint)rva - wBottomByteRealRVA;

    wMaxByteCountToRead = wVirtualRVA + 1 + 16;
    wBuffer.resize(wMaxByteCountToRead);

    mMemPage->read(wBuffer.data(), wBottomByteRealRVA, wBuffer.size());

    dsint addr = mDisasm->DisassembleBack((byte_t*)wBuffer.data(), rvaToVa(wBottomByteRealRVA), wBuffer.size(), wVirtualRVA, count);

    addr += rva - wVirtualRVA;

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
dsint Disassembly::getNextInstructionRVA(dsint rva, duint count, bool isGlobal)
{
    QByteArray wBuffer;
    dsint wRemainingBytes;
    dsint wMaxByteCountToRead;
    dsint wNewRVA;

    if(!isGlobal)
    {
        if(mMemPage->getSize() < (duint)rva)
            return rva;
        wRemainingBytes = mMemPage->getSize() - rva;

        wMaxByteCountToRead = 16 * (count + 1);
        if(mCodeFoldingManager)
            wMaxByteCountToRead += mCodeFoldingManager->getFoldedSize(rvaToVa(rva), rvaToVa(rva + wMaxByteCountToRead));
        wMaxByteCountToRead = wRemainingBytes > wMaxByteCountToRead ? wMaxByteCountToRead : wRemainingBytes;
    }
    else
    {
        wMaxByteCountToRead = 16 * (count + 1);
        if(mCodeFoldingManager)
            wMaxByteCountToRead += mCodeFoldingManager->getFoldedSize(rvaToVa(rva), rvaToVa(rva + wMaxByteCountToRead));
    }
    wBuffer.resize(wMaxByteCountToRead);

    mMemPage->read(wBuffer.data(), rva, wBuffer.size());

    wNewRVA = mDisasm->DisassembleNext((byte_t*)wBuffer.data(), rvaToVa(rva), wBuffer.size(), 0, count);

    wNewRVA += rva;

    return wNewRVA;
}

/**
 * @brief       Returns the RVA of count-th instructions before/after (depending on the sign) the given instruction RVA.
 *
 * @param[in]   rva         Instruction RVA
 * @param[in]   count       Instruction count
 *
 * @return      RVA of count-th instructions before/after the given instruction RVA.
 */
dsint Disassembly::getInstructionRVA(dsint index, dsint count)
{
    dsint wAddr = 0;

    if(count == 0)
        wAddr = index;
    if(count < 0)
        wAddr = getPreviousInstructionRVA(index, qAbs(count));
    else if(count > 0)
        wAddr = getNextInstructionRVA(index, qAbs(count));


    if(wAddr < 0)
        wAddr = 0;
    else if(wAddr > getRowCount() - 1)
        wAddr = getRowCount() - 1;

    return wAddr;
}

/**
 * @brief       Disassembles the instruction at the given RVA.
 *
 * @param[in]   rva     RVA of instruction to disassemble
 *
 * @return      Return the disassembled instruction.
 */
Instruction_t Disassembly::DisassembleAt(dsint rva)
{
    if(mMemPage->getSize() < (duint)rva)
        return Instruction_t();

    QByteArray wBuffer;
    duint base = mMemPage->getBase();
    duint wMaxByteCountToRead = 16 * 2;

    // Bounding
    auto size = getSize();
    if(!size)
        size = rva + wMaxByteCountToRead * 2;

    if(mCodeFoldingManager)
        wMaxByteCountToRead += mCodeFoldingManager->getFoldedSize(rvaToVa(rva), rvaToVa(rva + wMaxByteCountToRead));

    wMaxByteCountToRead = wMaxByteCountToRead > (size - rva) ? (size - rva) : wMaxByteCountToRead;
    if(!wMaxByteCountToRead)
        return Instruction_t();

    wBuffer.resize(wMaxByteCountToRead);

    if(!mMemPage->read(wBuffer.data(), rva, wBuffer.size()))
        return Instruction_t();

    return mDisasm->DisassembleAt((byte_t*)wBuffer.data(), wBuffer.size(), base, rva);
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
Instruction_t Disassembly::DisassembleAt(dsint rva, dsint count)
{
    rva = getNextInstructionRVA(rva, count);
    return DisassembleAt(rva);
}

/************************************************************************************
                                Selection Management
************************************************************************************/
void Disassembly::expandSelectionUpTo(dsint to)
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

void Disassembly::setSingleSelection(dsint index)
{
    mSelection.firstSelectedIndex = index;
    mSelection.fromIndex = index;
    mSelection.toIndex = getInstructionRVA(mSelection.fromIndex, 1) - 1;
    emit selectionChanged(rvaToVa(index));
}

dsint Disassembly::getInitialSelection() const
{
    return mSelection.firstSelectedIndex;
}

dsint Disassembly::getSelectionSize() const
{
    return mSelection.toIndex - mSelection.fromIndex + 1;
}

dsint Disassembly::getSelectionStart() const
{
    return mSelection.fromIndex;
}

dsint Disassembly::getSelectionEnd() const
{
    return mSelection.toIndex;
}

void Disassembly::selectionChangedSlot(dsint Va)
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
    dsint wAddr;
    dsint wStart = getInstructionRVA(getSelectionStart(), 1) - 1;
    dsint wInstrSize;
    if(expand)
    {
        if(getSelectionEnd() == getInitialSelection() && wStart != getSelectionEnd()) //decrease down
        {
            wAddr = getInstructionRVA(getSelectionStart(), 1);
            expandSelectionUpTo(wAddr);
        }
        else //expand down
        {
            wAddr = getSelectionEnd() + 1;
            wInstrSize = getInstructionRVA(wAddr, 1) - 1;
            expandSelectionUpTo(wInstrSize);
        }
    }
    else //select next instruction
    {
        wAddr = getSelectionEnd() + 1;
        setSingleSelection(wAddr);
        wInstrSize = getInstructionRVA(wAddr, 1) - 1;
        expandSelectionUpTo(wInstrSize);
    }
}

void Disassembly::selectPrevious(bool expand)
{
    dsint wAddr;
    dsint wStart;
    dsint wInstrSize;
    wStart = getInstructionRVA(getSelectionStart(), 1) - 1;
    if(expand)
    {
        if(getSelectionStart() == getInitialSelection() && wStart != getSelectionEnd()) //decrease up
        {
            wAddr = getInstructionRVA(getSelectionEnd() + 1, -2);
            wInstrSize = getInstructionRVA(wAddr, 1) - 1;
            expandSelectionUpTo(wInstrSize);
        }
        else //expand up
        {
            wAddr = getInstructionRVA(wStart + 1, -2);
            expandSelectionUpTo(wAddr);
        }
    }
    else
    {
        wAddr = getInstructionRVA(getSelectionStart(), -1);
        setSingleSelection(wAddr);
        wInstrSize = getInstructionRVA(wAddr, 1) - 1;
        expandSelectionUpTo(wInstrSize);
    }
}

bool Disassembly::isSelected(dsint base, dsint offset)
{
    dsint wAddr = base;

    if(offset < 0)
        wAddr = getPreviousInstructionRVA(getTableOffset(), offset);
    else if(offset > 0)
        wAddr = getNextInstructionRVA(getTableOffset(), offset);

    if(wAddr >= mSelection.fromIndex && wAddr <= mSelection.toIndex)
        return true;
    else
        return false;
}

bool Disassembly::isSelected(QList<Instruction_t>* buffer, int index) const
{
    if(buffer->size() > 0 && index >= 0 && index < buffer->size())
    {
        if((dsint)buffer->at(index).rva >= mSelection.fromIndex && (dsint)buffer->at(index).rva <= mSelection.toIndex)
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

void Disassembly::prepareDataCount(const QList<dsint> & wRVAs, QList<Instruction_t>* instBuffer)
{
    instBuffer->clear();
    Instruction_t wInst;
    for(int wI = 0; wI < wRVAs.count(); wI++)
    {
        wInst = DisassembleAt(wRVAs.at(wI));
        instBuffer->append(wInst);
    }
}

void Disassembly::prepareDataRange(dsint startRva, dsint endRva, const std::function<bool(int, const Instruction_t &)> & disassembled)
{
    dsint wAddrPrev = startRva;
    dsint wAddr = wAddrPrev;

    int i = 0;
    while(true)
    {
        if(wAddr > endRva)
            break;
        wAddrPrev = wAddr;
        auto wInst = DisassembleAt(wAddr);
        wAddr = getNextInstructionRVA(wAddr, 1);
        if(wAddr == wAddrPrev)
            break;
        if(!disassembled(i++, wInst))
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
            if((unsigned char)(instr.dump.at(byteIdx)) == patchInfo.newbyte)
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
    dsint wViewableRowsCount = getViewableRowsCount();
    mInstBuffer.clear();
    mInstBuffer.reserve(wViewableRowsCount);

    dsint wAddrPrev = getTableOffset();
    dsint wAddr = wAddrPrev;
    Instruction_t wInst;

    int wCount = 0;

    for(int wI = 0; wI < wViewableRowsCount && getRowCount() > 0; wI++)
    {
        wAddrPrev = wAddr;
        wInst = DisassembleAt(wAddr);
        wAddr = getNextInstructionRVA(wAddr, 1);
        if(wAddr == wAddrPrev)
            break;
        mInstBuffer.append(wInst);
        wCount++;
    }

    setNbrOfLineToPrint(wCount);

    mRichText.resize(getColumnCount());
    for(size_t i = 0; i < mRichText.size(); i++)
    {
        mRichText[i].resize(wViewableRowsCount);
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

#include <vtune.h>

void Disassembly::paintEvent(QPaintEvent* event)
{
    AbstractTableView::paintEvent(event);

    EnableVTune vtEnable;

    // Delay paint the rich text
    QPainter painter(this->viewport());
    painter.setFont(font());
    int x = -horizontalScrollBar()->value();

    for(int column = 0; column < mRichText.size(); column++)
    {
        int w = getColumnWidth(column);
        int h = getViewableRowsCount() * getRowHeight();

#if 1

        QString columnText;
        columnText.reserve(getColumnWidth(column) * getViewableRowsCount() / getCharWidth());

        QVector<QTextLayout::FormatRange> selections;

        for(int rowOffset = 0; rowOffset < mRichText[column].size(); rowOffset++)
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
                    //mFormatCache.resize(10);
                }

                QTextLayout::FormatRange range = std::move(mFormatCache.back());
                mFormatCache.pop_back();
                range.start = columnText.length();
                range.length = curRichText.text.length();

                columnText += curRichText.text;

                // TODO: this shit is way too slow
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
                /* TODO: underline
                painter->drawText(QRect(x + xinc, y, w - xinc, h), Qt::TextBypassShaping, curRichText.text);
                if(curRichText.underline && curRichText.underlineColor.alpha())
                {
                    highlightPen.setColor(curRichText.underlineColor);
                    highlightPen.setWidth(curRichText.underlineWidth);
                    painter->setPen(highlightPen);
                    int highlightOffsetX = curRichText.underlineConnectPrev ? -1 : 1;
                    painter->drawLine(x + xinc + highlightOffsetX, y + h - 1, x + xinc + backgroundWidth - 1, y + h - 1);
                }
                */
                selections.push_back(std::move(range));
            }

            //RichTextPainter::paintRichText(&painter, info.x, info.y, info.w, info.h, info.xinc, info.richText, mTextLayout);
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
            //line.setLineWidth(w);
            const RichTextInfo & info = mRichText[column][i];
            line.setPosition(QPointF(info.xinc, y));
        }

        mTextLayout.endLayout();

        QPixmap pixmap(w, h);
        pixmap.fill(Qt::transparent);

        QPainter clippedPainter;
        clippedPainter.begin(&pixmap);

        mTextLayout.draw(&clippedPainter, QPointF(0, 0));

        clippedPainter.end();

        //painter.drawPixmap(QRectF(x, 0, w, h), pixmap, QRectF(0, 0, w, h));
        painter.drawPixmap(x, 0, pixmap);

#else

        //painter.drawLine(x, 5, x + 10, 5);

        //qDebug() << "column:" << column << ", x:" << x << ", y:" << y << ", w:" << w << ", h:" << h;
        for(int rowOffset = 0; rowOffset < mRichText[column].size(); rowOffset++)
        {
            const RichTextInfo & info = mRichText[column][rowOffset];
            if(info.alive)
                RichTextPainter::paintRichText(&painter, info.x, info.y, info.w, info.h, info.xinc, info.richText, mFontMetrics);
        }
#endif

        x += w;
    }
}


/************************************************************************************
                        Public Methods
************************************************************************************/
duint Disassembly::rvaToVa(dsint rva) const
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

void Disassembly::disassembleAt(dsint parVA, bool history, dsint newTableOffset)
{
    duint wSize;
    auto wBase = DbgMemFindBaseAddr(parVA, &wSize);

    unsigned char test;
    if(!wBase || !wSize || !DbgMemRead(parVA, &test, sizeof(test)))
        return;
    dsint wRVA = parVA - wBase;

    HistoryData newHistory;

    //VA history
    if(history)
    {
        //truncate everything right from the current VA
        if(mVaHistory.size() && mCurrentVa < mVaHistory.size() - 1) //mCurrentVa is not the last
            mVaHistory.erase(mVaHistory.begin() + mCurrentVa + 1, mVaHistory.end());

        //NOTE: mCurrentVa always points to the last entry of the list

        //add the currently selected address to the history
        dsint selectionVA = rvaToVa(getInitialSelection()); //currently selected VA
        dsint selectionTableOffset = getTableOffset();
        if(selectionVA && mVaHistory.size() && mVaHistory.last().va != selectionVA) //do not have 2x the same va in a row
        {
            mCurrentVa++;
            newHistory.va = selectionVA;
            newHistory.tableOffset = selectionTableOffset;
            mVaHistory.push_back(newHistory);
        }
    }

    // Set base and size (Useful when memory page changed)
    mMemPage->setAttributes(wBase, wSize);
    mDisasm->getEncodeMap()->setMemoryRegion(wBase);

    if(mRvaDisplayEnabled && mMemPage->getBase() != mRvaDisplayPageBase)
        mRvaDisplayEnabled = false;

    setRowCount(wSize);

    setSingleSelection(wRVA);               // Selects disassembled instruction
    dsint wInstrSize = getInstructionRVA(wRVA, 1) - wRVA - 1;
    expandSelectionUpTo(wRVA + wInstrSize);

    if(newTableOffset == -1) //nothing specified
    {
        // Update table offset depending on the location of the instruction to disassemble
        if(mInstBuffer.size() > 0 && wRVA >= (dsint)mInstBuffer.first().rva && wRVA < (dsint)mInstBuffer.last().rva)
        {
            int wI;
            bool wIsAligned = false;

            // Check if the new RVA is aligned on an instruction from the cache (buffer)
            for(wI = 0; wI < mInstBuffer.size(); wI++)
            {
                if(mInstBuffer.at(wI).rva == wRVA)
                {
                    wIsAligned = true;
                    break;
                }
            }

            if(wIsAligned == true)
            {
                updateViewport();
            }
            else
            {
                setTableOffset(wRVA);
            }
        }
        else if(mInstBuffer.size() > 0 && wRVA == (dsint)mInstBuffer.last().rva)
        {
            setTableOffset(mInstBuffer.first().rva + mInstBuffer.first().length);
        }
        else
        {
            setTableOffset(wRVA);
        }

        if(history)
        {
            //new disassembled address
            newHistory.va = parVA;
            newHistory.tableOffset = getTableOffset();
            if(mVaHistory.size())
            {
                if(mVaHistory.last().va != parVA) //not 2x the same va in history
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

void Disassembly::disassembleAtSlot(dsint parVA, dsint parCIP)
{
    if(mCodeFoldingManager)
    {
        mCodeFoldingManager->expandFoldSegment(parVA);
        mCodeFoldingManager->expandFoldSegment(parCIP);
    }
    mCipVa = parCIP;
    if(mIsMain || !mMemPage->getBase())
        disassembleAt(parVA, true, -1);
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

QString Disassembly::getAddrText(dsint cur_addr, char label[MAX_LABEL_SIZE], bool getLabel)
{
    QString addrText = "";
    if(mRvaDisplayEnabled) //RVA display
    {
        dsint rva = cur_addr - mRvaDisplayBase;
        if(rva == 0)
        {
#ifdef _WIN64
            addrText = "$ ==>            ";
#else
            addrText = "$ ==>    ";
#endif //_WIN64
        }
        else if(rva > 0)
        {
#ifdef _WIN64
            addrText = "$+" + QString("%1").arg(rva, -15, 16, QChar(' ')).toUpper();
#else
            addrText = "$+" + QString("%1").arg(rva, -7, 16, QChar(' ')).toUpper();
#endif //_WIN64
        }
        else if(rva < 0)
        {
#ifdef _WIN64
            addrText = "$-" + QString("%1").arg(-rva, -15, 16, QChar(' ')).toUpper();
#else
            addrText = "$-" + QString("%1").arg(-rva, -7, 16, QChar(' ')).toUpper();
#endif //_WIN64
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
    if(label)
        strcpy_s(label, MAX_LABEL_SIZE, label_);
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
void Disassembly::unfold(dsint rva)
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
