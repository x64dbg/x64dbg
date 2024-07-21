#include "TraceBrowser.h"
#include "TraceWidget.h"
#include "TraceFileReader.h"
#include "TraceFileSearch.h"
#include "RichTextPainter.h"
#include "main.h"
#include "BrowseDialog.h"
#include "QZydis.h"
#include "GotoDialog.h"
#include "CommonActions.h"
#include "LineEditDialog.h"
#include "WordEditDialog.h"
#include "CachedFontMetrics.h"
#include "MRUList.h"
#include <QFileDialog>

TraceBrowser::TraceBrowser(TraceFileReader* traceFile, TraceWidget* parent) : AbstractTableView(parent), mTraceFile(traceFile)
{
    addColumnAt(getCharWidth() * 2 * 2 + 8, tr("Index"), false); //index
    addColumnAt(getCharWidth() * 2 * sizeof(dsint) + 8, tr("Address"), false); //address
    addColumnAt(getCharWidth() * 2 * 12 + 8, tr("Bytes"), false); //bytes
    addColumnAt(getCharWidth() * 40, tr("Disassembly"), false); //disassembly
    addColumnAt(getCharWidth() * 50, tr("Registers"), false); //registers
    addColumnAt(getCharWidth() * 50, tr("Memory"), false); //memory
    addColumnAt(1000, tr("Comments"), false); //comments
    loadColumnFromConfig("Trace");

    setShowHeader(false); //hide header

    mParent = parent;

    mSelection.firstSelectedIndex = 0;
    mSelection.fromIndex = 0;
    mSelection.toIndex = 0;
    setRowCount(0);
    mRvaDisplayBase = 0;
    mRvaDisplayEnabled = false;

    duint setting = 0;
    BridgeSettingGetUint("Gui", "TraceSyncCpu", &setting);
    mTraceSyncCpu = setting != 0;

    mHighlightingMode = false;
    mPermanentHighlightingMode = false;
    mShowMnemonicBrief = false;

    setupRightClickContextMenu();

    Initialize();

    connect(Bridge::getBridge(), SIGNAL(updateTraceBrowser()), this, SLOT(updateSlot()));
    connect(Bridge::getBridge(), SIGNAL(gotoTraceIndex(duint)), this, SLOT(gotoIndexSlot(duint)));

    connect(Config(), SIGNAL(tokenizerConfigUpdated()), this, SLOT(tokenizerConfigUpdatedSlot()));
    connect(this, SIGNAL(selectionChanged(unsigned long long)), this, SLOT(selectionChangedSlot(unsigned long long)));
    connect(Bridge::getBridge(), SIGNAL(close()), this, SLOT(closeFileSlot()));
    connect(getTraceFile(), SIGNAL(parseFinished()), this, SLOT(parseFinishedSlot()));
}

TraceBrowser::~TraceBrowser()
{
}

bool TraceBrowser::isFileOpened() const
{
    return mTraceFile && mTraceFile->Length() > 0;
}

bool TraceBrowser::isRecording()
{
    return DbgEval("tr.isrecording()") != 0;
}

bool TraceBrowser::toggleTraceRecording(QWidget* parent)
{
    if(!DbgIsDebugging())
        return false;
    if(isRecording())
    {
        return DbgCmdExecDirect("StopTraceRecording");
    }
    else
    {
        auto extension = ArchValue(".trace32", ".trace64");
        BrowseDialog browse(
            parent,
            tr("Start trace recording"),
            tr("Trace recording file"),
            tr("Trace recordings (*%1);;All files (*.*)").arg(extension),
            getDbPath(mainModuleName() + extension, true),
            true
        );
        if(browse.exec() == QDialog::Accepted)
        {
            if(browse.path.contains(QChar('"')) || browse.path.contains(QChar('\'')))
                SimpleErrorBox(parent, tr("Error"), tr("File name contains invalid character."));
            else
                return DbgCmdExecDirect(QString("StartTraceRecording \"%1\"").arg(browse.path));
        }
    }
    return false;
}

QString TraceBrowser::getAddrText(dsint cur_addr, char label[MAX_LABEL_SIZE], bool getLabel)
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
        if(DbgGetModuleAt(cur_addr, module) && !QString(label_).startsWith("JMP.&"))
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

//The following function is modified from "RichTextPainter::List Disassembly::getRichBytes(const Instruction_t & instr) const"
//with patch and code folding features removed.
RichTextPainter::List TraceBrowser::getRichBytes(const Instruction_t & instr) const
{
    RichTextPainter::List richBytes;
    std::vector<std::pair<size_t, bool>> realBytes;
    formatOpcodeString(instr, richBytes, realBytes);
    const duint cur_addr = instr.rva;

    if(!richBytes.empty() && richBytes.back().text.endsWith(' '))
        richBytes.back().text.chop(1); //remove trailing space if exists

    for(size_t i = 0; i < richBytes.size(); i++)
    {
        auto byteIdx = realBytes[i].first;
        auto isReal = realBytes[i].second;
        RichTextPainter::CustomRichText_t & curByte = richBytes.at(i);
        DBGRELOCATIONINFO relocInfo;
        curByte.underlineColor = mDisassemblyRelocationUnderlineColor;
        if(DbgIsDebugging() && DbgFunctions()->ModRelocationAtAddr(cur_addr + byteIdx, &relocInfo))
        {
            bool prevInSameReloc = relocInfo.rva < cur_addr + byteIdx - DbgFunctions()->ModBaseFromAddr(cur_addr + byteIdx);
            curByte.underline = isReal;
            curByte.underlineConnectPrev = i > 0 && prevInSameReloc;
        }
        else
        {
            curByte.underline = false;
            curByte.underlineConnectPrev = false;
        }

        curByte.textColor = mBytesColor;
        curByte.textBackground = mBytesBackgroundColor;
    }
    return richBytes;
}

#define HANDLE_RANGE_TYPE(prefix, first, last) \
    if(first == prefix ## _BEGIN && last == prefix ## _END) \
        first = prefix ## _SINGLE; \
    if(last == prefix ## _END && first != prefix ## _SINGLE) \
        first = last

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

int TraceBrowser::paintFunctionGraphic(QPainter* painter, int x, int y, Function_t funcType, bool loop)
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

QString TraceBrowser::paintContent(QPainter* painter, duint row, duint col, int x, int y, int w, int h)
{
    if(!isFileOpened())
    {
        return "";
    }
    QString reason;
    if(getTraceFile()->isError(reason))
    {
        GuiAddLogMessage(tr("An error occurred when reading trace file (reason: %1).\r\n").arg(reason).toUtf8().constData());
        emit closeFile();
        return "";
    }
    if(mHighlightingMode)
    {
        QPen pen(mInstructionHighlightColor);
        pen.setWidth(2);
        painter->setPen(pen);
        QRect rect = viewport()->rect();
        rect.adjust(1, 1, -1, -1);
        painter->drawRect(rect);
    }

    duint index = row;
    duint cur_addr;
    REGDUMP reg;
    reg = getTraceFile()->Registers(index);
    cur_addr = reg.regcontext.cip;
    auto traceCount = DbgFunctions()->GetTraceRecordHitCount(cur_addr);
    bool rowSelected = (index >= mSelection.fromIndex && index <= mSelection.toIndex);

    // Highlight if selected
    if(rowSelected && traceCount)
        painter->fillRect(QRect(x, y, w, h), QBrush(mTracedSelectedAddressBackgroundColor));
    else if(rowSelected)
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

    if(index >= getTraceFile()->Length())
        return "";

    Instruction_t inst = getTraceFile()->Instruction(index);

    switch(static_cast<TableColumnIndex>(col))
    {
    case Index:
    {
        return getTraceFile()->getIndexText(index);
    }

    case Address:
    {
        QString addrText;
        char label[MAX_LABEL_SIZE] = "";
        if(!DbgIsDebugging())
        {
            addrText = ToPtrString(cur_addr);
            goto NotDebuggingLabel;
        }
        else
            addrText = getAddrText(cur_addr, label, true);
        BPXTYPE bpxtype = DbgGetBpxTypeAt(cur_addr);
        bool isbookmark = DbgGetBookmarkAt(cur_addr);
        //todo: cip
        {
            if(!isbookmark) //no bookmark
            {
                if(*label) //label
                {
                    if(bpxtype == bp_none) //label only : fill label background
                    {
                        painter->setPen(mLabelColor); //red -> address + label text
                        painter->fillRect(QRect(x, y, w, h), QBrush(mLabelBackgroundColor)); //fill label background
                    }
                    else //label + breakpoint
                    {
                        if(bpxtype & bp_normal) //label + normal breakpoint
                        {
                            painter->setPen(mBreakpointColor);
                            painter->fillRect(QRect(x, y, w, h), QBrush(mBreakpointBackgroundColor)); //fill red
                        }
                        else if(bpxtype & bp_hardware) //label + hardware breakpoint only
                        {
                            painter->setPen(mHardwareBreakpointColor);
                            painter->fillRect(QRect(x, y, w, h), QBrush(mHardwareBreakpointBackgroundColor)); //fill ?
                        }
                        else //other cases -> do as normal
                        {
                            painter->setPen(mLabelColor); //red -> address + label text
                            painter->fillRect(QRect(x, y, w, h), QBrush(mLabelBackgroundColor)); //fill label background
                        }
                    }
                }
                else //no label
                {
                    if(bpxtype == bp_none) //no label, no breakpoint
                    {
NotDebuggingLabel:
                        QColor background;
                        if(rowSelected)
                        {
                            background = mSelectedAddressBackgroundColor;
                            painter->setPen(mSelectedAddressColor); //black address (DisassemblySelectedAddressColor)
                        }
                        else
                        {
                            background = mAddressBackgroundColor;
                            painter->setPen(mAddressColor); //DisassemblyAddressColor
                        }
                        if(background.alpha())
                            painter->fillRect(QRect(x, y, w, h), QBrush(background)); //fill background
                    }
                    else //breakpoint only
                    {
                        if(bpxtype & bp_normal) //normal breakpoint
                        {
                            painter->setPen(mBreakpointColor);
                            painter->fillRect(QRect(x, y, w, h), QBrush(mBreakpointBackgroundColor)); //fill red
                        }
                        else if(bpxtype & bp_hardware) //hardware breakpoint only
                        {
                            painter->setPen(mHardwareBreakpointColor);
                            painter->fillRect(QRect(x, y, w, h), QBrush(mHardwareBreakpointBackgroundColor)); //fill red
                        }
                        else //other cases (memory breakpoint in disassembly) -> do as normal
                        {
                            QColor background;
                            if(rowSelected)
                            {
                                background = mSelectedAddressBackgroundColor;
                                painter->setPen(mSelectedAddressColor); //black address (DisassemblySelectedAddressColor)
                            }
                            else
                            {
                                background = mAddressBackgroundColor;
                                painter->setPen(mAddressColor);
                            }
                            if(background.alpha())
                                painter->fillRect(QRect(x, y, w, h), QBrush(background)); //fill background
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
                        painter->setPen(mLabelColor); //red -> address + label text
                        painter->fillRect(QRect(x, y, w, h), QBrush(mBookmarkBackgroundColor)); //fill label background
                    }
                    else //label + breakpoint + bookmark
                    {
                        QColor color = mBookmarkBackgroundColor;
                        if(!color.alpha()) //we don't want transparent text
                            color = mAddressColor;
                        painter->setPen(color);
                        if(bpxtype & bp_normal) //label + bookmark + normal breakpoint
                        {
                            painter->fillRect(QRect(x, y, w, h), QBrush(mBreakpointBackgroundColor)); //fill red
                        }
                        else if(bpxtype & bp_hardware) //label + bookmark + hardware breakpoint only
                        {
                            painter->fillRect(QRect(x, y, w, h), QBrush(mHardwareBreakpointBackgroundColor)); //fill ?
                        }
                    }
                }
                else //bookmark, no label
                {
                    if(bpxtype == bp_none) //bookmark only
                    {
                        painter->setPen(mBookmarkColor); //black address
                        painter->fillRect(QRect(x, y, w, h), QBrush(mBookmarkBackgroundColor)); //fill bookmark color
                    }
                    else //bookmark + breakpoint
                    {
                        QColor color = mBookmarkBackgroundColor;
                        if(!color.alpha()) //we don't want transparent text
                            color = mAddressColor;
                        painter->setPen(color);
                        if(bpxtype & bp_normal) //bookmark + normal breakpoint
                        {
                            painter->fillRect(QRect(x, y, w, h), QBrush(mBreakpointBackgroundColor)); //fill red
                        }
                        else if(bpxtype & bp_hardware) //bookmark + hardware breakpoint only
                        {
                            painter->fillRect(QRect(x, y, w, h), QBrush(mHardwareBreakpointBackgroundColor)); //fill red
                        }
                        else //other cases (bookmark + memory breakpoint in disassembly) -> do as normal
                        {
                            painter->setPen(mBookmarkColor); //black address
                            painter->fillRect(QRect(x, y, w, h), QBrush(mBookmarkBackgroundColor)); //fill bookmark color
                        }
                    }
                }
            }
        }
        painter->drawText(QRect(x + 4, y, w - 4, h), Qt::AlignVCenter | Qt::AlignLeft, addrText);
    }
    return "";

    case Opcode:
    {
        int charwidth = getCharWidth();
        int funcsize = 0;
        if(DbgIsDebugging())
        {
            //draw functions
            Function_t funcType;
            FUNCTYPE funcFirst = DbgGetFunctionTypeAt(cur_addr);
            FUNCTYPE funcLast = DbgGetFunctionTypeAt(cur_addr + inst.length - 1);
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
            funcsize = paintFunctionGraphic(painter, x, y, funcType, false);

            painter->setPen(mFunctionPen);

            char indicator;
            XREFTYPE refType = DbgGetXrefTypeAt(cur_addr);
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

            painter->drawText(QRect(x + funcsize, y, charwidth, h), Qt::AlignVCenter | Qt::AlignLeft, QString(indicator));
        }
        funcsize += charwidth;

        //draw jump arrows
        Instruction_t::BranchType branchType = inst.branchType;
        painter->setPen(mConditionalTruePen);
        int halfRow = getRowHeight() / 2 + 1;
        int jumpsize = 0;
        if((branchType == Instruction_t::Conditional || branchType == Instruction_t::Unconditional) && index < getTraceFile()->Length())
        {
            duint next_addr;
            next_addr = getTraceFile()->Registers(index + 1).regcontext.cip;
            if(next_addr < cur_addr)
            {
                QPoint points[] =
                {
                    QPoint(x + funcsize, y + halfRow + 1),
                    QPoint(x + funcsize + 2, y + halfRow - 1),
                    QPoint(x + funcsize + 4, y + halfRow + 1),
                };
                jumpsize = 8;
                painter->drawPolyline(points, 3);
            }
            else if(next_addr > cur_addr)
            {
                QPoint points[] =
                {
                    QPoint(x + funcsize, y + halfRow - 1),
                    QPoint(x + funcsize + 2, y + halfRow + 1),
                    QPoint(x + funcsize + 4, y + halfRow - 1),
                };
                jumpsize = 8;
                painter->drawPolyline(points, 3);
            }
        }

        RichTextPainter::paintRichText(painter, x, y, getColumnWidth(col), getRowHeight(), jumpsize + funcsize, getRichBytes(inst), mFontMetrics);
        return "";
    }

    case Disassembly:
    {
        RichTextPainter::List richText;

        int loopsize = 0;
        int depth = 0;

        while(1) //paint all loop depths
        {
            LOOPTYPE loopFirst = DbgGetLoopTypeAt(cur_addr, depth);
            LOOPTYPE loopLast = DbgGetLoopTypeAt(cur_addr + inst.length - 1, depth);
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

        if(mHighlightToken.text.length())
            ZydisTokenizer::TokenToRichText(inst.tokens, richText, &mHighlightToken);
        else
            ZydisTokenizer::TokenToRichText(inst.tokens, richText, 0);
        RichTextPainter::paintRichText(painter, x + loopsize, y, getColumnWidth(col) - 0, getRowHeight(), 4, richText, mFontMetrics);
        return "";
    }

    case Registers:
    {
        RichTextPainter::List richText;
        auto fakeInstruction = registersTokens(index);
        if(mHighlightToken.text.length())
            ZydisTokenizer::TokenToRichText(fakeInstruction, richText, &mHighlightToken);
        else
            ZydisTokenizer::TokenToRichText(fakeInstruction, richText, 0);
        RichTextPainter::paintRichText(painter, x + 0, y, getColumnWidth(col) - 0, getRowHeight(), 4, richText, mFontMetrics);

        return "";
    }
    case Memory:
    {
        auto fakeInstruction = memoryTokens(index);
        RichTextPainter::List richText;
        if(mHighlightToken.text.length())
            ZydisTokenizer::TokenToRichText(fakeInstruction, richText, &mHighlightToken);
        else
            ZydisTokenizer::TokenToRichText(fakeInstruction, richText, nullptr);
        RichTextPainter::paintRichText(painter, x + 0, y, getColumnWidth(col) - 0, getRowHeight(), 4, richText, mFontMetrics);

        return "";
    }
    case Comments:
    {
        int xinc = 3;
        int width;
        if(DbgIsDebugging())
        {
            //TODO: draw arguments
            QString comment;
            bool autoComment = false;
            char label[MAX_LABEL_SIZE] = "";
            if(GetCommentFormat(cur_addr, comment, &autoComment))
            {
                QColor backgroundColor;
                if(autoComment)
                {
                    //TODO: autocomments from trace file will be much more helpful
                    painter->setPen(mAutoCommentColor);
                    backgroundColor = mAutoCommentBackgroundColor;
                }
                else //user comment
                {
                    painter->setPen(mCommentColor);
                    backgroundColor = mCommentBackgroundColor;
                }

                width = mFontMetrics->width(comment);
                if(width > w)
                    width = w;
                if(width)
                    painter->fillRect(QRect(x + xinc, y, width, h), QBrush(backgroundColor)); //fill comment color
                painter->drawText(QRect(x + xinc, y, width, h), Qt::AlignVCenter | Qt::AlignLeft, comment);
            }
            else if(DbgGetLabelAt(cur_addr, SEG_DEFAULT, label)) // label but no comment
            {
                QString labelText(label);
                QColor backgroundColor;
                painter->setPen(mLabelColor);
                backgroundColor = mLabelBackgroundColor;

                width = mFontMetrics->width(labelText);
                if(width > w)
                    width = w;
                if(width)
                    painter->fillRect(QRect(x + xinc, y, width, h), QBrush(backgroundColor)); //fill comment color
                painter->drawText(QRect(x + xinc, y, width, h), Qt::AlignVCenter | Qt::AlignLeft, labelText);
            }
            else
                width = 0;
            x += width + 3;
        }
        if(mShowMnemonicBrief)
        {
            char brief[MAX_STRING_SIZE] = "";
            QString mnem;
            for(const ZydisTokenizer::SingleToken & token : inst.tokens.tokens)
            {
                if(token.type != ZydisTokenizer::TokenType::Space && token.type != ZydisTokenizer::TokenType::Prefix)
                {
                    mnem = token.text;
                    break;
                }
            }
            if(mnem.isEmpty())
                mnem = inst.instStr;

            int index = mnem.indexOf(' ');
            if(index != -1)
                mnem.truncate(index);
            DbgFunctions()->GetMnemonicBrief(mnem.toUtf8().constData(), MAX_STRING_SIZE, brief);

            painter->setPen(mMnemonicBriefColor);

            QString mnemBrief = brief;
            if(mnemBrief.length())
            {
                width = mFontMetrics->width(mnemBrief);
                if(width > w)
                    width = w;
                if(width)
                    painter->fillRect(QRect(x, y, width, h), QBrush(mMnemonicBriefBackgroundColor)); //mnemonic brief background color
                painter->drawText(QRect(x, y, width, h), Qt::AlignVCenter | Qt::AlignLeft, mnemBrief);
            }
        }
        return "";
    }

    default:
        return "";
    }
}

ZydisTokenizer::InstructionToken TraceBrowser::memoryTokens(unsigned long long atIndex)
{
    duint MemoryAddress[MAX_MEMORY_OPERANDS];
    duint MemoryOldContent[MAX_MEMORY_OPERANDS];
    duint MemoryNewContent[MAX_MEMORY_OPERANDS];
    bool MemoryIsValid[MAX_MEMORY_OPERANDS];
    int MemoryOperandsCount;
    ZydisTokenizer::InstructionToken fakeInstruction = ZydisTokenizer::InstructionToken();

    MemoryOperandsCount = getTraceFile()->MemoryAccessCount(atIndex);
    if(MemoryOperandsCount > 0)
    {
        getTraceFile()->MemoryAccessInfo(atIndex, MemoryAddress, MemoryOldContent, MemoryNewContent, MemoryIsValid);
        std::vector<ZydisTokenizer::SingleToken> tokens;

        for(int i = 0; i < MemoryOperandsCount; i++)
        {
            ZydisTokenizer::TokenizeTraceMemory(MemoryAddress[i], MemoryOldContent[i], MemoryNewContent[i], tokens);
        }


        fakeInstruction.tokens.insert(fakeInstruction.tokens.begin(), tokens.begin(), tokens.end());
    }
    return  fakeInstruction;
}

ZydisTokenizer::InstructionToken TraceBrowser::registersTokens(unsigned long long atIndex)
{
    ZydisTokenizer::InstructionToken fakeInstruction = ZydisTokenizer::InstructionToken();
    REGDUMP now = getTraceFile()->Registers(atIndex);
    REGDUMP next = (atIndex + 1 < getTraceFile()->Length()) ? getTraceFile()->Registers(atIndex + 1) : now;
    std::vector<ZydisTokenizer::SingleToken> tokens;

#define addRegValues(str, reg) if (atIndex ==0 || now.regcontext.##reg != next.regcontext.##reg) { \
    ZydisTokenizer::TokenizeTraceRegister(str, now.regcontext.##reg, next.regcontext.##reg, tokens);};

    addRegValues(ArchValue("eax", "rax"), cax)
    addRegValues(ArchValue("ebx", "rbx"), cbx)
    addRegValues(ArchValue("ecx", "rcx"), ccx)
    addRegValues(ArchValue("edx", "rdx"), cdx)
    addRegValues(ArchValue("esp", "rsp"), csp)
    addRegValues(ArchValue("ebp", "rbp"), cbp)
    addRegValues(ArchValue("esi", "rsi"), csi)
    addRegValues(ArchValue("edi", "rdi"), cdi)
#ifdef _WIN64
    addRegValues("r8", r8)
    addRegValues("r9", r9)
    addRegValues("r10", r10)
    addRegValues("r11", r11)
    addRegValues("r12", r12)
    addRegValues("r13", r13)
    addRegValues("r14", r14)
    addRegValues("r15", r15)
#endif //_WIN64
    addRegValues(ArchValue("eflags", "rflags"), eflags)

    fakeInstruction.tokens.insert(fakeInstruction.tokens.begin(), tokens.begin(), tokens.end());
    return fakeInstruction;
}

void TraceBrowser::prepareData()
{
    auto viewables = getViewableRowsCount();
    int lines = 0;
    if(isFileOpened())
    {
        duint tableOffset = getTableOffset();
        if(getTraceFile()->Length() < tableOffset + viewables)
            lines = getTraceFile()->Length() - tableOffset;
        else
            lines = viewables;
    }
    setNbrOfLineToPrint(lines);
}

void TraceBrowser::setupRightClickContextMenu()
{
    mMenuBuilder = new MenuBuilder(this);
    mCommonActions = new CommonActions(this, getActionHelperFuncs(), [this]()
    {
        return getTraceFile()->Registers(getInitialSelection()).regcontext.cip;
    });

    auto mTraceFileNotNull = [](QMenu*)
    {
        return true; // This should always be true now
    };
    auto isDebugging = [](QMenu*)
    {
        return DbgIsDebugging();
    };

    MenuBuilder* copyMenu = new MenuBuilder(this, mTraceFileNotNull);
    copyMenu->addAction(makeShortcutAction(DIcon("copy_selection"), tr("&Selection"), SLOT(copySelectionSlot()), "ActionCopy"));
    copyMenu->addAction(makeAction(DIcon("copy_selection"), tr("Selection to &File"), SLOT(copySelectionToFileSlot())));
    copyMenu->addAction(makeAction(DIcon("copy_selection_no_bytes"), tr("Selection (&No Bytes)"), SLOT(copySelectionNoBytesSlot())));
    copyMenu->addAction(makeAction(DIcon("copy_selection_no_bytes"), tr("Selection to File (No Bytes)"), SLOT(copySelectionToFileNoBytesSlot())));
    copyMenu->addAction(makeShortcutAction(DIcon("database-export"), tr("&Export Table"), SLOT(exportSlot()), "ActionExport"));
    copyMenu->addAction(makeShortcutAction(DIcon("copy_address"), tr("Address"), SLOT(copyCipSlot()), "ActionCopyAddress"));
    copyMenu->addAction(makeShortcutAction(DIcon("copy_address"), tr("&RVA"), SLOT(copyRvaSlot()), "ActionCopyRva"), isDebugging);
    copyMenu->addAction(makeShortcutAction(DIcon("fileoffset"), tr("&File Offset"), SLOT(copyFileOffsetSlot()), "ActionCopyFileOffset"), isDebugging);
    copyMenu->addAction(makeAction(DIcon("copy_disassembly"), tr("Disassembly"), SLOT(copyDisassemblySlot())));
    copyMenu->addAction(makeAction(DIcon("copy_address"), tr("Index"), SLOT(copyIndexSlot())));

    mMenuBuilder->addMenu(makeMenu(DIcon("copy"), tr("&Copy")), copyMenu);

    mCommonActions->build(mMenuBuilder, CommonActions::ActionDisasm | CommonActions::ActionBreakpoint | CommonActions::ActionLabel | CommonActions::ActionComment | CommonActions::ActionBookmark);
    mMenuBuilder->addAction(makeShortcutAction(DIcon("highlight"), tr("&Highlighting mode"), SLOT(enableHighlightingModeSlot()), "ActionHighlightingMode"), mTraceFileNotNull);
    mMenuBuilder->addAction(makeShortcutAction(DIcon("helpmnemonic"), tr("Help on mnemonic"), SLOT(mnemonicHelpSlot()), "ActionHelpOnMnemonic"), mTraceFileNotNull);
    QAction* mnemonicBrief = makeShortcutAction(DIcon("helpbrief"), tr("Show mnemonic brief"), SLOT(mnemonicBriefSlot()), "ActionToggleMnemonicBrief");
    mMenuBuilder->addAction(mnemonicBrief, [this, mnemonicBrief](QMenu*)
    {
        if(mShowMnemonicBrief)
            mnemonicBrief->setText(tr("Hide mnemonic brief"));
        else
            mnemonicBrief->setText(tr("Show mnemonic brief"));
        return true;
    });
    MenuBuilder* gotoMenu = new MenuBuilder(this, mTraceFileNotNull);
    gotoMenu->addAction(makeShortcutAction(DIcon("geolocation-goto"), tr("Expression"), SLOT(gotoSlot()), "ActionGotoExpression"), mTraceFileNotNull);
    gotoMenu->addAction(makeAction(DIcon("goto"), tr("Index"), SLOT(gotoIndexSlot())), mTraceFileNotNull);
    gotoMenu->addAction(makeAction(DIcon("arrow-step-rtr"), tr("Function return"), SLOT(rtrSlot())), mTraceFileNotNull);
    gotoMenu->addAction(makeShortcutAction(DIcon("previous"), tr("Previous"), SLOT(gotoPreviousSlot()), "ActionGotoPrevious"), [this](QMenu*)
    {
        return mHistory.historyHasPrev();
    });
    gotoMenu->addAction(makeShortcutAction(DIcon("next"), tr("Next"), SLOT(gotoNextSlot()), "ActionGotoNext"), [this](QMenu*)
    {
        return mHistory.historyHasNext();
    });
    mMenuBuilder->addMenu(makeMenu(DIcon("goto"), tr("Go to")), gotoMenu);

    mMenuBuilder->addAction(makeShortcutAction(DIcon("xrefs"), tr("xrefs..."), SLOT(gotoXrefSlot()), "ActionXrefs"));

    MenuBuilder* searchMenu = new MenuBuilder(this, mTraceFileNotNull);
    searchMenu->addAction(makeAction(DIcon("search_for_constant"), tr("Address/Constant"), SLOT(searchConstantSlot())));
    searchMenu->addAction(makeAction(DIcon("memory-map"), tr("Memory Reference"), SLOT(searchMemRefSlot())));
    mMenuBuilder->addMenu(makeMenu(DIcon("search"), tr("&Search")), searchMenu);

    // The following code adds a menu to view the information about currently selected instruction. When info box is completed, remove me.
    MenuBuilder* infoMenu = new MenuBuilder(this, [this](QMenu * menu)
    {
        duint MemoryAddress[MAX_MEMORY_OPERANDS];
        duint MemoryOldContent[MAX_MEMORY_OPERANDS];
        duint MemoryNewContent[MAX_MEMORY_OPERANDS];
        bool MemoryIsValid[MAX_MEMORY_OPERANDS];
        int MemoryOperandsCount;
        unsigned long long index;

        if(!isFileOpened())
            return false;
        index = getInitialSelection();
        MemoryOperandsCount = getTraceFile()->MemoryAccessCount(index);
        if(MemoryOperandsCount > 0)
        {
            getTraceFile()->MemoryAccessInfo(index, MemoryAddress, MemoryOldContent, MemoryNewContent, MemoryIsValid);
            bool RvaDisplayEnabled = mRvaDisplayEnabled;
            char nolabel[MAX_LABEL_SIZE];
            mRvaDisplayEnabled = false;
            for(int i = 0; i < MemoryOperandsCount; i++)
                menu->addAction(QString("%1: %2 -> %3").arg(getAddrText(MemoryAddress[i], nolabel, false)).arg(ToPtrString(MemoryOldContent[i])).arg(ToPtrString(MemoryNewContent[i])));
            mRvaDisplayEnabled = RvaDisplayEnabled;
            return true;
        }
        else
            return false; //The information menu now only contains memory access info
    });
    mMenuBuilder->addMenu(makeMenu(tr("Information")), infoMenu);

    auto synchronizeCpuAction = makeShortcutAction(DIcon("sync"), tr("Sync with CPU"), SLOT(synchronizeCpuSlot()), "ActionSync");
    synchronizeCpuAction->setCheckable(true);
    synchronizeCpuAction->setChecked(mTraceSyncCpu);
    mMenuBuilder->addAction(synchronizeCpuAction);

    mMenuBuilder->addSeparator();
    QAction* toggleTraceRecording = makeShortcutAction(DIcon("control-record"), tr("Start recording"), SLOT(toggleTraceRecordingSlot()), "ActionToggleRunTrace");
    mMenuBuilder->addAction(toggleTraceRecording, [toggleTraceRecording](QMenu*)
    {
        if(!DbgIsDebugging())
            return false;
        if(isRecording())
        {
            toggleTraceRecording->setText(tr("Stop recording"));
            toggleTraceRecording->setIcon(DIcon("control-stop"));
        }
        else
        {
            toggleTraceRecording->setText(tr("Start recording"));
            toggleTraceRecording->setIcon(DIcon("control-record"));
        }
        return true;
    });
    mMenuBuilder->addAction(makeAction(DIcon("close"), tr("Close recording"), SLOT(closeFileSlot())), mTraceFileNotNull)
    ->setStatusTip(tr("Close the trace file tab, and stop recording trace."));
    mMenuBuilder->addAction(makeAction(DIcon("delete"), tr("Delete recording"), SLOT(closeDeleteSlot())), mTraceFileNotNull)
    ->setStatusTip(tr("Delete the trace file from disk, and stop recording trace."));
    mMenuBuilder->addAction(makeShortcutAction(DIcon("browseinexplorer"), tr("Browse in Explorer"), SLOT(browseInExplorerSlot()), "ActionBrowseInExplorer"), mTraceFileNotNull)
    ->setStatusTip(tr("Open the trace file in Explorer."));

    mMenuBuilder->loadFromConfig();
}

void TraceBrowser::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu(this);
    mMenuBuilder->build(&menu);
    menu.exec(event->globalPos());
}

void TraceBrowser::mousePressEvent(QMouseEvent* event)
{
    auto index = getIndexOffsetFromY(transY(event->y())) + getTableOffset();
    if(getGuiState() != AbstractTableView::NoState || !getTraceFile())
    {
        AbstractTableView::mousePressEvent(event);
        return;
    }
    switch(event->button())
    {
    case Qt::LeftButton:
        if(index < getRowCount())
        {
            if(mHighlightingMode || mPermanentHighlightingMode)
            {
                ZydisTokenizer::InstructionToken tokens;
                int columnPosition = 0;
                if(getColumnIndexFromX(event->x()) == Disassembly)
                {
                    tokens = getTraceFile()->Instruction(index).tokens;
                    columnPosition = getColumnPosition(Disassembly);
                }
                else if(getColumnIndexFromX(event->x()) == TableColumnIndex::Registers)
                {
                    tokens = registersTokens(index);
                    columnPosition = getColumnPosition(Registers);
                }
                else if(getColumnIndexFromX(event->x()) == Memory)
                {
                    tokens = memoryTokens(index);
                    columnPosition = getColumnPosition(Memory);
                }
                ZydisTokenizer::SingleToken token;
                if(ZydisTokenizer::TokenFromX(tokens, token, event->x() - columnPosition, mFontMetrics))
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
            if(mHighlightingMode) //disable highlighting mode after clicked
            {
                mHighlightingMode = false;
                reloadData();
            }
            if(event->modifiers() & Qt::ShiftModifier)
                expandSelectionUpTo(index);
            else
                setSingleSelection(index);
            mHistory.addVaToHistory(index);
            emit selectionChanged(getInitialSelection());
        }
        updateViewport();
        return;

        break;
    case Qt::MiddleButton:
        copyCipSlot();
        MessageBeep(MB_OK);
        break;
    case Qt::BackButton:
        gotoPreviousSlot();
        break;
    case Qt::ForwardButton:
        gotoNextSlot();
        break;
    }

    AbstractTableView::mousePressEvent(event);
}

void TraceBrowser::mouseDoubleClickEvent(QMouseEvent* event)
{
    if(event->button() == Qt::LeftButton && getTraceFile() != nullptr)
    {
        switch(getColumnIndexFromX(event->x()))
        {
        case Index://Index: follow
            mCommonActions->followDisassemblySlot();
            break;
        case Address://Address: set RVA
            if(mRvaDisplayEnabled && getTraceFile()->Registers(getInitialSelection()).regcontext.cip == mRvaDisplayBase)
                mRvaDisplayEnabled = false;
            else
            {
                mRvaDisplayEnabled = true;
                mRvaDisplayBase = getTraceFile()->Registers(getInitialSelection()).regcontext.cip;
            }
            reloadData();
            break;
        case Opcode: //Opcode: Breakpoint
            mCommonActions->toggleInt3BPActionSlot();
            break;
        case Disassembly: //Instructions: follow
            mCommonActions->followDisassemblySlot();
            break;
        case Comments: //Comment
            mCommonActions->setCommentSlot();
            break;
        }
    }
    AbstractTableView::mouseDoubleClickEvent(event);
}

void TraceBrowser::mouseMoveEvent(QMouseEvent* event)
{
    auto index = getIndexOffsetFromY(transY(event->y())) + getTableOffset();
    if((event->buttons() & Qt::LeftButton) != 0 && getGuiState() == AbstractTableView::NoState && getTraceFile() != nullptr)
    {
        if(index < getRowCount())
        {
            setSingleSelection(getInitialSelection());
            expandSelectionUpTo(index);
        }
        if(transY(event->y()) > this->height())
        {
            verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepAdd);
        }
        else if(transY(event->y()) < 0)
        {
            verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepSub);
        }
        updateViewport();
    }
    AbstractTableView::mouseMoveEvent(event);
}

void TraceBrowser::keyPressEvent(QKeyEvent* event)
{
    int key = event->key();
    auto curindex = getInitialSelection();
    auto visibleindex = curindex;
    if((key == Qt::Key_Up || key == Qt::Key_Down) && getTraceFile())
    {
        if(key == Qt::Key_Up)
        {
            if(event->modifiers() == Qt::ShiftModifier)
            {
                if(curindex == getSelectionStart())
                {
                    if(getSelectionEnd() > 0)
                    {
                        visibleindex = getSelectionEnd() - 1;
                        expandSelectionUpTo(visibleindex);
                    }
                }
                else
                {
                    if(getSelectionStart() > 0)
                    {
                        visibleindex = getSelectionStart() - 1;
                        expandSelectionUpTo(visibleindex);
                    }
                }
            }
            else
            {
                if(curindex > 0)
                {
                    visibleindex = curindex - 1;
                    setSingleSelection(visibleindex);
                }
            }
        }
        else
        {
            if(getSelectionEnd() + 1 < getTraceFile()->Length())
            {
                if(event->modifiers() == Qt::ShiftModifier)
                {
                    visibleindex = getSelectionEnd() + 1;
                    expandSelectionUpTo(visibleindex);
                }
                else
                {
                    visibleindex = getSelectionEnd() + 1;
                    setSingleSelection(visibleindex);
                }
            }
        }
        makeVisible(visibleindex);
        mHistory.addVaToHistory(visibleindex);
        updateViewport();

        emit selectionChanged(getInitialSelection());
    }
    else
        AbstractTableView::keyPressEvent(event);
}

void TraceBrowser::selectionChangedSlot(unsigned long long selection)
{
    if(mTraceSyncCpu && isFileOpened())
    {
        GuiDisasmAt(getTraceFile()->Registers(selection).regcontext.cip, 0);
    }
}

void TraceBrowser::tokenizerConfigUpdatedSlot()
{
    mPermanentHighlightingMode = ConfigBool("Disassembler", "PermanentHighlightingMode");
}

void TraceBrowser::expandSelectionUpTo(duint to)
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

void TraceBrowser::setSingleSelection(duint index)
{
    mSelection.firstSelectedIndex = index;
    mSelection.fromIndex = index;
    mSelection.toIndex = index;
}

duint TraceBrowser::getInitialSelection()
{
    return mSelection.firstSelectedIndex;
}

duint TraceBrowser::getSelectionSize()
{
    return mSelection.toIndex - mSelection.fromIndex + 1;
}

duint TraceBrowser::getSelectionStart()
{
    return mSelection.fromIndex;
}

duint TraceBrowser::getSelectionEnd()
{
    return mSelection.toIndex;
}

void TraceBrowser::makeVisible(duint index)
{
    duint tableOffset = getTableOffset();
    if(index < tableOffset)
        setTableOffset(index);
    else if(index + 2 > tableOffset + getViewableRowsCount())
        setTableOffset(index - getViewableRowsCount() + 2);
}

void TraceBrowser::updateColors()
{
    AbstractTableView::updateColors();
    //ZydisTokenizer::UpdateColors(); //Already called in disassembly
    mBackgroundColor = ConfigColor("DisassemblyBackgroundColor");

    mInstructionHighlightColor = ConfigColor("InstructionHighlightColor");
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
    mAutoCommentColor = ConfigColor("DisassemblyAutoCommentColor");
    mAutoCommentBackgroundColor = ConfigColor("DisassemblyAutoCommentBackgroundColor");
    mMnemonicBriefColor = ConfigColor("DisassemblyMnemonicBriefColor");
    mMnemonicBriefBackgroundColor = ConfigColor("DisassemblyMnemonicBriefBackgroundColor");
    mCommentColor = ConfigColor("DisassemblyCommentColor");
    mCommentBackgroundColor = ConfigColor("DisassemblyCommentBackgroundColor");
    mConditionalJumpLineTrueColor = ConfigColor("DisassemblyConditionalJumpLineTrueColor");
    mDisassemblyRelocationUnderlineColor = ConfigColor("DisassemblyRelocationUnderlineColor");
    mLoopColor = ConfigColor("DisassemblyLoopColor");
    mFunctionColor = ConfigColor("DisassemblyFunctionColor");

    auto a = mSelectionColor, b = mTracedAddressBackgroundColor;
    mTracedSelectedAddressBackgroundColor = QColor((a.red() + b.red()) / 2, (a.green() + b.green()) / 2, (a.blue() + b.blue()) / 2);

    mLoopPen = QPen(mLoopColor, 2);
    mFunctionPen = QPen(mFunctionColor, 2);
    mConditionalTruePen = QPen(mConditionalJumpLineTrueColor);
}

void TraceBrowser::openFileSlot()
{
    BrowseDialog browse(
        this,
        tr("Open trace recording"),
        tr("Trace recording"),
        tr("Trace recordings (*.%1);;All files (*.*)").arg(ArchValue("trace32", "trace64")),
        getDbPath(),
        false
    );
    if(browse.exec() != QDialog::Accepted)
        return;
    emit openSlot(browse.path);
}

void TraceBrowser::openSlot(const QString & fileName)
{
    GuiOpenTraceFile(fileName.toUtf8().constData()); // Open in Trace Manager
}

void TraceBrowser::browseInExplorerSlot()
{
    QStringList arguments;
    arguments << QString("/select,");
    arguments << QString(mTraceFile->FileName());
    QProcess::startDetached(QString("%1/explorer.exe").arg(QProcessEnvironment::systemEnvironment().value("windir")), arguments);
}

void TraceBrowser::toggleTraceRecordingSlot()
{
    toggleTraceRecording(this);
}

void TraceBrowser::closeFileSlot()
{
    if(isRecording())
        DbgCmdExecDirect("StopTraceRecording");
    emit closeFile();
}

void TraceBrowser::closeDeleteSlot()
{
    QMessageBox msgbox(QMessageBox::Critical, tr("Delete recording"), tr("Are you sure you want to delete this recording?"), QMessageBox::Yes | QMessageBox::No, this);
    if(msgbox.exec() == QMessageBox::Yes)
    {
        if(isRecording())
            DbgCmdExecDirect("StopTraceRecording");
        mTraceFile->Delete();
        emit closeFile();
    }
}

void TraceBrowser::parseFinishedSlot()
{
    QString reason;
    if(mTraceFile->isError(reason))
    {
        // Trace widget will display an error message and close the tab. Here we don't do it again.
        setRowCount(0);
    }
    else
    {
        setRowCount(mTraceFile->Length());
    }
    setSingleSelection(0);
    makeVisible(0);
    emit Bridge::getBridge()->updateTraceBrowser();
    emit selectionChanged(getInitialSelection());
}

void TraceBrowser::mnemonicBriefSlot()
{
    mShowMnemonicBrief = !mShowMnemonicBrief;
    reloadData();
}

void TraceBrowser::mnemonicHelpSlot()
{
    unsigned char data[16] = { 0xCC };
    int size;
    getTraceFile()->OpCode(getInitialSelection(), data, &size);
    Zydis zydis;
    zydis.Disassemble(getTraceFile()->Registers(getInitialSelection()).regcontext.cip, data);
    DbgCmdExecDirect(QString("mnemonichelp %1").arg(zydis.Mnemonic().c_str()));
    emit displayLogWidget();
}

void TraceBrowser::disasm(unsigned long long index, bool history)
{
    setSingleSelection(index);
    makeVisible(index);
    if(history)
        mHistory.addVaToHistory(index);
    updateViewport();
    emit selectionChanged(getInitialSelection());
}

void TraceBrowser::disasmByAddress(duint address, bool history)
{
    mParent->loadDumpFully();
    auto references = getTraceFile()->getDump()->getReferences(address, address);
    unsigned long long index;
    bool found = false;
    if(references.empty())
    {
        QString addr = ToPtrString(address);
        QMessageBox msg(this);
        msg.setIcon(QMessageBox::Warning);
        msg.setWindowTitle(tr("Address not found in trace"));
        if(DbgIsDebugging())
        {
            msg.setText(tr("The address %1 is not found in trace.").arg(addr) + ' ' + tr("Do you want to follow in CPU instead?"));
            msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            if(msg.exec() == QMessageBox::Yes)
            {
                DbgCmdExec(QString("disasm %1").arg(addr));
            }
        }
        else
        {
            msg.setText(tr("The address %1 is not found in trace.").arg(addr));
            msg.setStandardButtons(QMessageBox::Ok);
            msg.exec();
        }
    }
    else
    {
        for(auto i : references)
        {
            if(getTraceFile()->Registers(i).regcontext.cip == address)
            {
                if(found == false)
                {
                    found = true;
                    index = i;
                }
                else
                {
                    // Multiple results, display the Xref dialog
                    emit xrefSignal(address);
                    return;
                }

            }
        }
        if(found)
        {
            disasm(index, history);
        }
        else
        {
            // There is no instruction execution, show the user some other types of memory access
            emit xrefSignal(address);
        }
    }
}

void TraceBrowser::gotoIndexSlot()
{
    if(getTraceFile() == nullptr)
        return;
    GotoDialog gotoDlg(this, false, true, true);
    if(gotoDlg.exec() == QDialog::Accepted)
    {
        auto val = DbgValFromString(gotoDlg.expressionText.toUtf8().constData());
        if(val >= 0 && val < getTraceFile()->Length())
            disasm(val);
    }
}

void TraceBrowser::gotoSlot()
{
    if(getTraceFile() == nullptr)
        return;
    GotoDialog gotoDlg(this, false, true, true);
    if(gotoDlg.exec() == QDialog::Accepted)
    {
        auto val = DbgValFromString(gotoDlg.expressionText.toUtf8().constData());
        disasmByAddress(val);
    }
}

void TraceBrowser::rtrSlot()
{
    // Let's hope this search will be fast...
    disasm(TraceFileSearchFuncReturn(getTraceFile(), getInitialSelection()));
}

void TraceBrowser::gotoNextSlot()
{
    if(mHistory.historyHasNext())
        disasm(mHistory.historyNext(), false);
}

void TraceBrowser::gotoPreviousSlot()
{
    if(mHistory.historyHasPrev())
        disasm(mHistory.historyPrev(), false);
}


void TraceBrowser::gotoXrefSlot()
{
    emit xrefSignal(getTraceFile()->Registers(getInitialSelection()).regcontext.cip);
}

void TraceBrowser::copyCipSlot()
{
    QString clipboard;
    for(auto i = getSelectionStart(); i <= getSelectionEnd(); i++)
    {
        if(i != getSelectionStart())
            clipboard += "\r\n";
        clipboard += ToPtrString(getTraceFile()->Registers(i).regcontext.cip);
    }
    Bridge::CopyToClipboard(clipboard);
}

void TraceBrowser::copyIndexSlot()
{
    QString clipboard;
    for(auto i = getSelectionStart(); i <= getSelectionEnd(); i++)
    {
        if(i != getSelectionStart())
            clipboard += "\r\n";
        clipboard += getTraceFile()->getIndexText(i);
    }
    Bridge::CopyToClipboard(clipboard);
}

void TraceBrowser::pushSelectionInto(bool copyBytes, QTextStream & stream, QTextStream* htmlStream)
{
    const int addressLen = getColumnWidth(Address) / getCharWidth() - 1;
    const int bytesLen = getColumnWidth(Opcode) / getCharWidth() - 1;
    const int disassemblyLen = getColumnWidth(Disassembly) / getCharWidth() - 1;
    const int registersLen = getColumnWidth(Registers) / getCharWidth() - 1;
    const int memoryLen = getColumnWidth(Memory) / getCharWidth() - 1;
    if(htmlStream)
        *htmlStream << QString("<table style=\"border-width:0px;border-color:#000000;font-family:%1;font-size:%2px;\">").arg(font().family()).arg(getRowHeight());
    for(unsigned long long i = getSelectionStart(); i <= getSelectionEnd(); i++)
    {
        if(i != getSelectionStart())
            stream << "\r\n";
        const Instruction_t & inst = getTraceFile()->Instruction(i);
        duint cur_addr = inst.rva;
        QString address = getAddrText(cur_addr, 0, addressLen > sizeof(duint) * 2 + 1);
        QString bytes;
        QString bytesHTML;
        if(copyBytes)
            RichTextPainter::htmlRichText(getRichBytes(inst), &bytesHTML, bytes);
        QString disassembly;
        QString htmlDisassembly;
        if(htmlStream)
        {
            RichTextPainter::List richText;
            if(mHighlightToken.text.length())
                ZydisTokenizer::TokenToRichText(inst.tokens, richText, &mHighlightToken);
            else
                ZydisTokenizer::TokenToRichText(inst.tokens, richText, 0);
            RichTextPainter::htmlRichText(richText, &htmlDisassembly, disassembly);
        }
        else
        {
            for(const auto & token : inst.tokens.tokens)
                disassembly += token.text;
        }
        QString fullComment;
        QString comment;
        bool autocomment;
        if(GetCommentFormat(cur_addr, comment, &autocomment))
            fullComment = " " + comment;

        QString registersText;
        QString registersHtml;
        ZydisTokenizer::InstructionToken regTokens = registersTokens(i);
        if(htmlStream)
        {
            RichTextPainter::List richText;
            if(mHighlightToken.text.length())
                ZydisTokenizer::TokenToRichText(regTokens, richText, &mHighlightToken);
            else
                ZydisTokenizer::TokenToRichText(regTokens, richText, 0);
            RichTextPainter::htmlRichText(richText, &registersHtml, registersText);
        }
        else
        {
            for(const auto & token : regTokens.tokens)
                registersText += token.text;
        }

        QString memoryText;
        QString memoryHtml;
        ZydisTokenizer::InstructionToken memTokens = memoryTokens(i);
        if(htmlStream)
        {
            RichTextPainter::List richText;
            if(mHighlightToken.text.length())
                ZydisTokenizer::TokenToRichText(memTokens, richText, &mHighlightToken);
            else
                ZydisTokenizer::TokenToRichText(memTokens, richText, 0);
            RichTextPainter::htmlRichText(richText, &memoryHtml, memoryText);
        }
        else
        {
            for(const auto & token : memTokens.tokens)
                memoryText += token.text;
        }

        stream << getTraceFile()->getIndexText(i) + " | " + address.leftJustified(addressLen, QChar(' '), true);
        if(copyBytes)
            stream << " | " + bytes.leftJustified(bytesLen, QChar(' '), true);
        stream << " | " + disassembly.leftJustified(disassemblyLen, QChar(' '), true);
        stream << " | " + registersText.leftJustified(registersLen, QChar(' '), true);
        stream << " | " + memoryText.leftJustified(memoryLen, QChar(' '), true) + " |" + fullComment;
        if(htmlStream)
        {
            *htmlStream << QString("<tr><td>%1</td><td>%2</td><td>").arg(getTraceFile()->getIndexText(i), address.toHtmlEscaped());
            if(copyBytes)
                *htmlStream << QString("%1</td><td>").arg(bytesHTML);
            *htmlStream << QString("%1</td><td>").arg(htmlDisassembly);
            *htmlStream << QString("%1</td><td>").arg(registersText);
            *htmlStream << QString("%1</td><td>").arg(memoryText);
            if(!comment.isEmpty())
            {
                if(autocomment)
                {
                    *htmlStream << QString("<span style=\"color:%1").arg(mAutoCommentColor.name());
                    if(mAutoCommentBackgroundColor != Qt::transparent)
                        *htmlStream << QString(";background-color:%2").arg(mAutoCommentBackgroundColor.name());
                }
                else
                {
                    *htmlStream << QString("<span style=\"color:%1").arg(mCommentColor.name());
                    if(mCommentBackgroundColor != Qt::transparent)
                        *htmlStream << QString(";background-color:%2").arg(mCommentBackgroundColor.name());
                }
                *htmlStream << "\">" << comment.toHtmlEscaped() << "</span></td></tr>";
            }
            else
            {
                char label[MAX_LABEL_SIZE];
                if(DbgGetLabelAt(cur_addr, SEG_DEFAULT, label))
                {
                    comment = QString::fromUtf8(label);
                    *htmlStream << QString("<span style=\"color:%1").arg(mLabelColor.name());
                    if(mLabelBackgroundColor != Qt::transparent)
                        *htmlStream << QString(";background-color:%2").arg(mLabelBackgroundColor.name());
                    *htmlStream << "\">" << comment.toHtmlEscaped() << "</span></td></tr>";
                }
                else
                    *htmlStream << "</td></tr>";
            }
        }
    }
    if(htmlStream)
        *htmlStream << "</table>";
}

void TraceBrowser::copySelectionSlot(bool copyBytes)
{
    if(getTraceFile() == nullptr)
        return;

    QString selectionString = "";
    QString selectionHtmlString = "";
    QTextStream stream(&selectionString);
    if(getSelectionEnd() - getSelectionStart() < 2048)
    {
        QTextStream htmlStream(&selectionHtmlString);
        pushSelectionInto(copyBytes, stream, &htmlStream);
        Bridge::CopyToClipboard(selectionString, selectionHtmlString);
    }
    else
    {
        pushSelectionInto(copyBytes, stream, nullptr);
        Bridge::CopyToClipboard(selectionString);
    }
}

void TraceBrowser::copySelectionToFileSlot(bool copyBytes)
{
    if(getTraceFile() == nullptr)
        return;

    QString fileName = QFileDialog::getSaveFileName(this, tr("Open File"), "", tr("Text Files (*.txt)"));
    if(fileName != "")
    {
        QFile file(fileName);
        if(!file.open(QIODevice::WriteOnly))
        {
            QMessageBox::critical(this, tr("Error"), tr("Could not open file"));
            return;
        }

        QTextStream stream(&file);
        pushSelectionInto(copyBytes, stream);
        file.close();
    }
}

void TraceBrowser::copySelectionSlot()
{
    copySelectionSlot(true);
}

void TraceBrowser::copySelectionToFileSlot()
{
    copySelectionToFileSlot(true);
}

void TraceBrowser::copySelectionNoBytesSlot()
{
    copySelectionSlot(false);
}

void TraceBrowser::copySelectionToFileNoBytesSlot()
{
    copySelectionToFileSlot(false);
}

void TraceBrowser::copyDisassemblySlot()
{
    if(getTraceFile() == nullptr)
        return;

    QString clipboard = "";
    if(getSelectionEnd() - getSelectionStart() < 2048)
    {
        QString clipboardHtml = QString("<div style=\"font-family: %1; font-size: %2px\">").arg(font().family()).arg(getRowHeight());
        for(auto i = getSelectionStart(); i <= getSelectionEnd(); i++)
        {
            if(i != getSelectionStart())
            {
                clipboard += "\r\n";
                clipboardHtml += "<br/>";
            }
            RichTextPainter::List richText;
            const Instruction_t & inst = getTraceFile()->Instruction(i);
            ZydisTokenizer::TokenToRichText(inst.tokens, richText, 0);
            RichTextPainter::htmlRichText(richText, &clipboardHtml, clipboard);
        }
        clipboardHtml += QString("</div>");
        Bridge::CopyToClipboard(clipboard, clipboardHtml);
    }
    else
    {
        for(auto i = getSelectionStart(); i <= getSelectionEnd(); i++)
        {
            if(i != getSelectionStart())
            {
                clipboard += "\r\n";
            }
            RichTextPainter::List richText;
            const Instruction_t & inst = getTraceFile()->Instruction(i);
            ZydisTokenizer::TokenToRichText(inst.tokens, richText, 0);
            RichTextPainter::htmlRichText(richText, nullptr, clipboard);
        }
        Bridge::CopyToClipboard(clipboard);
    }
}

void TraceBrowser::copyRvaSlot()
{
    QString text;
    if(getTraceFile() == nullptr)
        return;

    for(unsigned long long i = getSelectionStart(); i <= getSelectionEnd(); i++)
    {
        duint cip = getTraceFile()->Registers(i).regcontext.cip;
        duint base = DbgFunctions()->ModBaseFromAddr(cip);
        if(base)
        {
            if(i != getSelectionStart())
                text += "\r\n";
            text += ToHexString(cip - base);
        }
        else
        {
            SimpleWarningBox(this, tr("Error!"), tr("Selection not in a module..."));
            return;
        }
    }
    Bridge::CopyToClipboard(text);
}

void TraceBrowser::copyFileOffsetSlot()
{
    QString text;
    if(getTraceFile() == nullptr)
        return;

    for(unsigned long long i = getSelectionStart(); i <= getSelectionEnd(); i++)
    {
        duint cip = getTraceFile()->Registers(i).regcontext.cip;
        cip = DbgFunctions()->VaToFileOffset(cip);
        if(cip)
        {
            if(i != getSelectionStart())
                text += "\r\n";
            text += ToHexString(cip);
        }
        else
        {
            SimpleErrorBox(this, tr("Error!"), tr("Selection not in a file..."));
            return;
        }
    }
    Bridge::CopyToClipboard(text);
}

void TraceBrowser::exportSlot()
{
    if(getTraceFile() == nullptr)
        return;
    std::vector<QString> headers;
    headers.reserve(getColumnCount());
    for(duint i = 0; i < getColumnCount(); i++)
        headers.push_back(getColTitle(i));
    ExportCSV(getRowCount(), getColumnCount(), headers, [this](dsint row, dsint col)
    {
        QString temp;
        switch(col)
        {
        case Index:
            return getTraceFile()->getIndexText(row);

        case Address:
        {
            if(!DbgIsDebugging())
                return ToPtrString(getTraceFile()->Registers(row).regcontext.cip);
            else
                return getAddrText(getTraceFile()->Registers(row).regcontext.cip, 0, true);
        }

        case Opcode:
        {
            for(auto i : getRichBytes(getTraceFile()->Instruction(row)))
                temp += i.text;
            return temp;
        }

        case Disassembly:
        {
            for(auto i : getTraceFile()->Instruction(row).tokens.tokens)
                temp += i.text;
            return temp;
        }

        case Registers:
        {
            for(auto i : registersTokens(row).tokens)
                temp += i.text;
            return temp;
        }
        case Memory:
        {
            for(auto i : memoryTokens(row).tokens)
                temp += i.text;
            return temp;
        }
        case Comments:
        {
            if(DbgIsDebugging())
            {
                //TODO: draw arguments
                QString comment;
                bool autoComment = false;
                char label[MAX_LABEL_SIZE] = "";
                if(GetCommentFormat(getTraceFile()->Registers(row).regcontext.cip, comment, &autoComment))
                {
                    return QString(comment);
                }
                else if(DbgGetLabelAt(getTraceFile()->Registers(row).regcontext.cip, SEG_DEFAULT, label)) // label but no comment
                {
                    return QString(label);
                }
            }
            return QString();
        }
        default:
            return QString();
        }
    });
}

void TraceBrowser::enableHighlightingModeSlot()
{
    if(mHighlightingMode)
        mHighlightingMode = false;
    else
        mHighlightingMode = true;
    reloadData();
}

void TraceBrowser::searchConstantSlot()
{
    if(!isFileOpened())
        return;
    WordEditDialog constantDlg(this);
    duint initialConstant = getTraceFile()->Registers(getInitialSelection()).regcontext.cip;
    constantDlg.setup(tr("Constant"), initialConstant, sizeof(duint));
    if(constantDlg.exec() == QDialog::Accepted)
    {
        auto ticks = GetTickCount();
        int count = TraceFileSearchConstantRange(getTraceFile(), constantDlg.getVal(), constantDlg.getVal());
        GuiShowReferences();
        GuiAddLogMessage(tr("%1 result(s) in %2ms\n").arg(count).arg(GetTickCount() - ticks).toUtf8().constData());
    }
}

void TraceBrowser::searchMemRefSlot()
{
    WordEditDialog memRefDlg(this);
    memRefDlg.setup(tr("References"), 0, sizeof(duint));
    if(memRefDlg.exec() == QDialog::Accepted)
    {
        auto ticks = GetTickCount();
        mParent->loadDumpFully();
        int count = TraceFileSearchMemReference(getTraceFile(), memRefDlg.getVal());
        GuiShowReferences();
        GuiAddLogMessage(tr("%1 result(s) in %2ms\n").arg(count).arg(GetTickCount() - ticks).toUtf8().constData());
    }
}

void TraceBrowser::updateSlot()
{
    if(getTraceFile()) // && this->isVisible()
    {
        if(isRecording())
        {
            getTraceFile()->purgeLastPage();
            setRowCount(getTraceFile()->Length());
        }
    }
    else
        setRowCount(0);
    reloadData();
}

void TraceBrowser::synchronizeCpuSlot()
{
    mTraceSyncCpu = !mTraceSyncCpu;
    BridgeSettingSetUint("Gui", "TraceSyncCpu", mTraceSyncCpu);
    selectionChangedSlot(getSelectionStart());
}

void TraceBrowser::gotoIndexSlot(duint index)
{
    disasm(index, false);
}

void TraceBrowser::gotoAddressSlot(duint address)
{
    disasmByAddress(address, false);
}
