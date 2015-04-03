#include "Disassembly.h"
#include "Configuration.h"
#include "Bridge.h"

Disassembly::Disassembly(QWidget* parent) : AbstractTableView(parent)
{
    fontsUpdated();
    mMemPage = new MemoryPage(0, 0);

    mInstBuffer.clear();

    historyClear();

    SelectionData_t data;
    memset(&data, 0, sizeof(SelectionData_t));
    mSelection = data;

    mCipRva = 0;
    mIsRunning = false;

    mHighlightToken.text = "";
    mHighlightingMode = false;

    mDisasm = new QBeaEngine();

    mIsLastInstDisplayed = false;

    mGuiState = Disassembly::NoState;

    setRowCount(mMemPage->getSize());

    addColumnAt(getCharWidth() * 2 * sizeof(int_t) + 8, "", false); //address
    addColumnAt(getCharWidth() * 2 * 12 + 8, "", false); //bytes
    addColumnAt(getCharWidth() * 40, "", false); //disassembly
    addColumnAt(1000, "", false); //comments

    setShowHeader(false); //hide header

    backgroundColor = ConfigColor("DisassemblyBackgroundColor");

    connect(Bridge::getBridge(), SIGNAL(repaintGui()), this, SLOT(reloadData()));
}

void Disassembly::colorsUpdated()
{
    AbstractTableView::colorsUpdated();
    backgroundColor = ConfigColor("DisassemblyBackgroundColor");
}

void Disassembly::fontsUpdated()
{
    setFont(ConfigFont("Disassembly"));
}

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
QString Disassembly::paintContent(QPainter* painter, int_t rowBase, int rowOffset, int col, int x, int y, int w, int h)
{
    Q_UNUSED(rowBase)
    if(mHighlightingMode)
    {
        QPen pen(ConfigColor("InstructionHighlightColor"));
        pen.setWidth(2);
        painter->setPen(pen);
        QRect rect = viewport()->rect();
        rect.adjust(1, 1, -1, -1);
        painter->drawRect(rect);
    }
    int_t wRVA = mInstBuffer.at(rowOffset).rva;
    bool wIsSelected = isSelected(&mInstBuffer, rowOffset);

    // Highlight if selected
    if(wIsSelected)
        painter->fillRect(QRect(x, y, w, h), QBrush(ConfigColor("DisassemblySelectionColor")));

    switch(col)
    {
    case 0: // Draw address (+ label)
    {
        char label[MAX_LABEL_SIZE] = "";
        int_t cur_addr = rvaToVa(mInstBuffer.at(rowOffset).rva);
        QString addrText = getAddrText(cur_addr, label);
        BPXTYPE bpxtype = DbgGetBpxTypeAt(cur_addr);
        bool isbookmark = DbgGetBookmarkAt(cur_addr);
        if(mInstBuffer.at(rowOffset).rva == mCipRva && !mIsRunning) //cip + not running
        {
            painter->fillRect(QRect(x, y, w, h), QBrush(ConfigColor("DisassemblyCipBackgroundColor")));
            if(!isbookmark) //no bookmark
            {
                if(bpxtype & bp_normal) //normal breakpoint
                {
                    QColor bpColor = ConfigColor("DisassemblyBreakpointBackgroundColor");
                    if(!bpColor.alpha()) //we don't want transparent text
                        bpColor = ConfigColor("DisassemblyBreakpointColor");
                    if(bpColor == ConfigColor("DisassemblyCipBackgroundColor"))
                        bpColor = ConfigColor("DisassemblyCipColor");
                    painter->setPen(QPen(bpColor));
                }
                else if(bpxtype & bp_hardware) //hardware breakpoint only
                {
                    QColor hwbpColor = ConfigColor("DisassemblyHardwareBreakpointBackgroundColor");
                    if(!hwbpColor.alpha()) //we don't want transparent text
                        hwbpColor = ConfigColor("DisassemblyHardwareBreakpointColor");
                    if(hwbpColor == ConfigColor("DisassemblyCipBackgroundColor"))
                        hwbpColor = ConfigColor("DisassemblyCipColor");
                    painter->setPen(hwbpColor);
                }
                else //no breakpoint
                {
                    painter->setPen(QPen(ConfigColor("DisassemblyCipColor")));
                }
            }
            else //bookmark
            {
                QColor bookmarkColor = ConfigColor("DisassemblyBookmarkBackgroundColor");
                if(!bookmarkColor.alpha()) //we don't want transparent text
                    bookmarkColor = ConfigColor("DisassemblyBookmarkColor");
                if(bookmarkColor == ConfigColor("DisassemblyCipBackgroundColor"))
                    bookmarkColor = ConfigColor("DisassemblyCipColor");
                painter->setPen(QPen(bookmarkColor));
            }
        }
        else //non-cip address
        {
            if(!isbookmark) //no bookmark
            {
                if(*label) //label
                {
                    if(bpxtype == bp_none) //label only
                    {
                        painter->setPen(QPen(ConfigColor("DisassemblyLabelColor"))); //red -> address + label text
                        painter->fillRect(QRect(x, y, w, h), QBrush(ConfigColor("DisassemblyLabelBackgroundColor"))); //fill label background
                    }
                    else //label+breakpoint
                    {
                        if(bpxtype & bp_normal) //label + normal breakpoint
                        {
                            painter->setPen(QPen(ConfigColor("DisassemblyBreakpointColor")));
                            painter->fillRect(QRect(x, y, w, h), QBrush(ConfigColor("DisassemblyBreakpointBackgroundColor"))); //fill red
                        }
                        else if(bpxtype & bp_hardware) //label + hardware breakpoint only
                        {
                            painter->setPen(QPen(ConfigColor("DisassemblyHardwareBreakpointColor")));
                            painter->fillRect(QRect(x, y, w, h), QBrush(ConfigColor("DisassemblyHardwareBreakpointBackgroundColor"))); //fill ?
                        }
                        else //other cases -> do as normal
                        {
                            painter->setPen(QPen(ConfigColor("DisassemblyLabelColor"))); //red -> address + label text
                            painter->fillRect(QRect(x, y, w, h), QBrush(ConfigColor("DisassemblyLabelBackgroundColor"))); //fill label background
                        }
                    }
                }
                else //no label
                {
                    if(bpxtype == bp_none) //no label, no breakpoint
                    {
                        QColor background;
                        if(wIsSelected)
                        {
                            background = ConfigColor("DisassemblySelectedAddressBackgroundColor");
                            painter->setPen(QPen(ConfigColor("DisassemblySelectedAddressColor"))); //black address (DisassemblySelectedAddressColor)
                        }
                        else
                        {
                            background = ConfigColor("DisassemblyAddressBackgroundColor");
                            painter->setPen(QPen(ConfigColor("DisassemblyAddressColor"))); //DisassemblyAddressColor
                        }
                        if(background.alpha())
                            painter->fillRect(QRect(x, y, w, h), QBrush(background)); //fill background
                    }
                    else //breakpoint only
                    {
                        if(bpxtype & bp_normal) //normal breakpoint
                        {
                            painter->setPen(QPen(ConfigColor("DisassemblyBreakpointColor")));
                            painter->fillRect(QRect(x, y, w, h), QBrush(ConfigColor("DisassemblyBreakpointBackgroundColor"))); //fill red
                        }
                        else if(bpxtype & bp_hardware) //hardware breakpoint only
                        {
                            painter->setPen(QPen(ConfigColor("DisassemblyHardwareBreakpointColor")));
                            painter->fillRect(QRect(x, y, w, h), QBrush(ConfigColor("DisassemblyHardwareBreakpointBackgroundColor"))); //fill red
                        }
                        else //other cases (memory breakpoint in disassembly) -> do as normal
                        {
                            QColor background;
                            if(wIsSelected)
                            {
                                background = ConfigColor("DisassemblySelectedAddressBackgroundColor");
                                painter->setPen(QPen(ConfigColor("DisassemblySelectedAddressColor"))); //black address (DisassemblySelectedAddressColor)
                            }
                            else
                            {
                                background = ConfigColor("DisassemblyAddressBackgroundColor");
                                painter->setPen(QPen(ConfigColor("DisassemblyAddressColor")));
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
                        painter->setPen(QPen(ConfigColor("DisassemblyLabelColor"))); //red -> address + label text
                        painter->fillRect(QRect(x, y, w, h), QBrush(ConfigColor("DisassemblyBookmarkBackgroundColor"))); //fill label background
                    }
                    else //label+breakpoint+bookmark
                    {
                        QColor color = ConfigColor("DisassemblyBookmarkBackgroundColor");
                        if(!color.alpha()) //we don't want transparent text
                            color = ConfigColor("DisassemblyAddressColor");
                        painter->setPen(QPen(color));
                        if(bpxtype & bp_normal) //label + bookmark + normal breakpoint
                        {
                            painter->fillRect(QRect(x, y, w, h), QBrush(ConfigColor("DisassemblyBreakpointBackgroundColor"))); //fill red
                        }
                        else if(bpxtype & bp_hardware) //label + bookmark + hardware breakpoint only
                        {
                            painter->fillRect(QRect(x, y, w, h), QBrush(ConfigColor("DisassemblyHardwareBreakpointBackgroundColor"))); //fill ?
                        }
                    }
                }
                else //bookmark, no label
                {
                    if(bpxtype == bp_none) //bookmark only
                    {
                        painter->setPen(QPen(ConfigColor("DisassemblyBookmarkColor"))); //black address
                        painter->fillRect(QRect(x, y, w, h), QBrush(ConfigColor("DisassemblyBookmarkBackgroundColor"))); //fill bookmark color
                    }
                    else //bookmark + breakpoint
                    {
                        QColor color = ConfigColor("DisassemblyBookmarkBackgroundColor");
                        if(!color.alpha()) //we don't want transparent text
                            color = ConfigColor("DisassemblyAddressColor");
                        painter->setPen(QPen(color));
                        if(bpxtype & bp_normal) //bookmark + normal breakpoint
                        {
                            painter->fillRect(QRect(x, y, w, h), QBrush(ConfigColor("DisassemblyBreakpointBackgroundColor"))); //fill red
                        }
                        else if(bpxtype & bp_hardware) //bookmark + hardware breakpoint only
                        {
                            painter->fillRect(QRect(x, y, w, h), QBrush(ConfigColor("DisassemblyHardwareBreakpointBackgroundColor"))); //fill red
                        }
                        else //other cases (bookmark + memory breakpoint in disassembly) -> do as normal
                        {
                            painter->setPen(QPen(ConfigColor("DisassemblyBookmarkColor"))); //black address (DisassemblySelectedAddressColor)
                            painter->fillRect(QRect(x, y, w, h), QBrush(ConfigColor("DisassemblyBookmarkBackgroundColor"))); //fill bookmark color
                        }
                    }
                }
            }
        }
        painter->drawText(QRect(x + 4, y , w - 4 , h), Qt::AlignVCenter | Qt::AlignLeft, addrText);
    }
    break;

    case 1: //draw bytes (TODO: some spaces between bytes)
    {
        //draw functions
        int_t cur_addr = rvaToVa(mInstBuffer.at(rowOffset).rva);
        Function_t funcType;
        FUNCTYPE funcFirst = DbgGetFunctionTypeAt(cur_addr);
        FUNCTYPE funcLast = DbgGetFunctionTypeAt(cur_addr + mInstBuffer.at(rowOffset).length - 1);
        if(funcLast == FUNC_END)
            funcFirst = funcLast;
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

        //draw jump arrows
        int jumpsize = paintJumpsGraphic(painter, x + funcsize, y, wRVA); //jump line

        //draw bytes
        QColor bytesColor = ConfigColor("DisassemblyBytesColor");
        QColor patchedBytesColor = ConfigColor("DisassemblyModifiedBytesColor");
        QList<RichTextPainter::CustomRichText_t> richBytes;
        RichTextPainter::CustomRichText_t space;
        space.highlight = false;
        space.flags = RichTextPainter::FlagNone;
        space.text = " ";
        RichTextPainter::CustomRichText_t curByte;
        curByte.highlight = false;
        curByte.flags = RichTextPainter::FlagColor;
        for(int i = 0; i < mInstBuffer.at(rowOffset).dump.size(); i++)
        {
            if(i)
                richBytes.push_back(space);
            curByte.text = QString("%1").arg((unsigned char)(mInstBuffer.at(rowOffset).dump.at(i)), 2, 16, QChar('0')).toUpper();
            curByte.textColor = DbgFunctions()->PatchGet(cur_addr + i) ? patchedBytesColor : bytesColor;
            richBytes.push_back(curByte);
        }
        RichTextPainter::paintRichText(painter, x, y, getColumnWidth(col), getRowHeight(), jumpsize + funcsize, &richBytes, getCharWidth());
    }
    break;

    case 2: //draw disassembly (with colours needed)
    {
        int_t cur_addr = rvaToVa(mInstBuffer.at(rowOffset).rva);
        int loopsize = 0;
        int depth = 0;

        while(1) //paint all loop depths
        {
            LOOPTYPE loopType = DbgGetLoopTypeAt(cur_addr, depth);
            if(loopType == LOOP_NONE)
                break;
            Function_t funcType;
            switch(loopType)
            {
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
            loopsize += paintFunctionGraphic(painter, x + loopsize, y, funcType, true);
            depth++;
        }

        QList<RichTextPainter::CustomRichText_t> richText;

        BeaTokenizer::BeaInstructionToken* token = &mInstBuffer[rowOffset].tokens;
        if(mHighlightToken.text.length())
            BeaTokenizer::TokenToRichText(token, &richText, &mHighlightToken);
        else
            BeaTokenizer::TokenToRichText(token, &richText, 0);
        int xinc = 4;
        RichTextPainter::paintRichText(painter, x + loopsize, y, getColumnWidth(col) - loopsize, getRowHeight(), xinc, &richText, getCharWidth());
        token->x = x + loopsize + xinc;
    }
    break;

    case 3: //draw comments
    {
        char comment[MAX_COMMENT_SIZE] = "";
        if(DbgGetCommentAt(rvaToVa(mInstBuffer.at(rowOffset).rva), comment))
        {
            QString commentText;
            QColor penColor;
            QColor backgroundColor;
            if(comment[0] == '\1') //automatic comment
            {
                penColor = ConfigColor("DisassemblyAutoCommentColor");
                backgroundColor = ConfigColor("DisassemblyAutoCommentBackgroundColor");
                commentText = QString(comment + 1);
            }
            else //user comment
            {
                penColor = ConfigColor("DisassemblyCommentColor");
                backgroundColor = ConfigColor("DisassemblyCommentBackgroundColor");
                commentText = comment;
            }
            painter->setPen(penColor);
            int width = getCharWidth() * commentText.length() + 4;
            if(width > w)
                width = w;
            if(width)
                painter->fillRect(QRect(x + 2, y, width, h), QBrush(backgroundColor)); //fill comment color
            painter->drawText(QRect(x + 4, y , w - 4 , h), Qt::AlignVCenter | Qt::AlignLeft, commentText);
        }
    }
    break;

    default:
        break;
    }
    return "";
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

    if(mGuiState == Disassembly::MultiRowsSelectionState)
    {
        //qDebug() << "State = MultiRowsSelectionState";

        if((transY(event->y()) >= 0) && (transY(event->y()) <= this->getTableHeigth()))
        {
            int wI = getIndexOffsetFromY(transY(event->y()));

            if(mMemPage->getSize() > 0)
            {
                // Bound
                wI = wI >= mInstBuffer.size() ? mInstBuffer.size() - 1 : wI;
                wI = wI < 0 ? 0 : wI;

                int_t wRowIndex = mInstBuffer.at(wI).rva;
                int_t wInstrSize = getInstructionRVA(wRowIndex, 1) - wRowIndex - 1;

                if(wRowIndex < getRowCount())
                {
                    setSingleSelection(getInitialSelection());
                    expandSelectionUpTo(getInstructionRVA(getInitialSelection(), 1) - 1);
                    if(wRowIndex > getInitialSelection()) //select down
                        expandSelectionUpTo(wRowIndex + wInstrSize);
                    else
                        expandSelectionUpTo(wRowIndex);

                    repaint();

                    wAccept = false;
                }
            }
        }
    }

    if(wAccept == true)
        AbstractTableView::mouseMoveEvent(event);
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

    if(DbgIsDebugging() && ((event->buttons() & Qt::LeftButton) != 0) && ((event->buttons() & Qt::RightButton) == 0))
    {
        if(getGuiState() == AbstractTableView::NoState)
        {
            if(mHighlightingMode)
            {
                if(getColumnIndexFromX(event->x()) == 2) //click in instruction column
                {
                    int rowOffset = getIndexOffsetFromY(transY(event->y()));
                    if(rowOffset < mInstBuffer.size())
                    {
                        BeaTokenizer::BeaSingleToken token;
                        if(BeaTokenizer::TokenFromX(&mInstBuffer.at(rowOffset).tokens, &token, event->x(), getCharWidth()))
                        {
                            if(BeaTokenizer::IsHighlightableToken(&token) && !BeaTokenizer::TokenEquals(&token, &mHighlightToken))
                                mHighlightToken = token;
                            else
                            {
                                mHighlightToken.value.value = 0;
                                mHighlightToken.text = "";
                            }
                        }
                        else
                        {
                            mHighlightToken.value.value = 0;
                            mHighlightToken.text = "";
                        }
                    }
                }
                else
                {
                    mHighlightToken.value.value = 0;
                    mHighlightToken.text = "";
                }
            }
            else if(event->y() > getHeaderHeight())
            {
                int_t wRowIndex = getInstructionRVA(getTableOffset(), getIndexOffsetFromY(transY(event->y())));
                int_t wInstrSize = getInstructionRVA(wRowIndex, 1) - wRowIndex - 1;

                if(wRowIndex < getRowCount())
                {
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

                    repaint();

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

            repaint();

            wAccept = false;
        }
    }

    if(wAccept == true)
        AbstractTableView::mouseReleaseEvent(event);
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

    if(key == Qt::Key_Up || key == Qt::Key_Down)
    {
        int_t botRVA = getTableOffset();
        int_t topRVA = getInstructionRVA(getTableOffset(), getNbrOfLineToPrint() - 1);

        bool expand = false;
        if(event->modifiers() & Qt::ShiftModifier) //SHIFT pressed
            expand = true;

        if(key == Qt::Key_Up)
            selectPrevious(expand);
        else
            selectNext(expand);

        if(getSelectionStart() < botRVA)
        {
            setTableOffset(getSelectionStart());
        }
        else if(getSelectionEnd() >= topRVA)
        {
            setTableOffset(getInstructionRVA(getSelectionEnd(), -getNbrOfLineToPrint() + 2));
        }

        repaint();
    }
    else if(key == Qt::Key_Return || key == Qt::Key_Enter)
    {
        uint_t dest = DbgGetBranchDestination(rvaToVa(getInitialSelection()));
        if(!dest)
            return;
        QString cmd = "disasm " + QString("%1").arg(dest, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
        DbgCmdExec(cmd.toUtf8().constData());
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
int_t Disassembly::sliderMovedHook(int type, int_t value, int_t delta)
{
    int_t wNewValue;

    if(type == QAbstractSlider::SliderNoAction) // QAbstractSlider::SliderNoAction is used to disassembe at a specific address
    {
        wNewValue = value + delta;
    }
    else if(type == QAbstractSlider::SliderMove) // If it's a slider action, disassemble one instruction back and one instruction next in order to be aligned on a real instruction
    {
        if(value + delta > 0)
        {
            wNewValue = getInstructionRVA(value + delta, -1);
            wNewValue = getInstructionRVA(wNewValue, 1);
        }
        else
            wNewValue = 0;
    }
    else // For other actions, disassemble according to the delta
    {
        wNewValue = getInstructionRVA(value, delta);
    }

    return wNewValue;
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
int Disassembly::paintJumpsGraphic(QPainter* painter, int x, int y, int_t addr)
{
    int_t selHeadRVA = mSelection.fromIndex;
    int_t rva = addr;
    Instruction_t instruction = DisassembleAt(selHeadRVA);
    Int32 branchType = instruction.disasm.Instruction.BranchType;

    GraphicDump_t wPict = GD_Nothing;

    if(branchType && branchType != RetType && branchType != CallType)
    {
        int_t destRVA = (int_t)DbgGetBranchDestination(rvaToVa(instruction.rva));

        int_t base = mMemPage->getBase();
        if(destRVA >= base && destRVA < base + (int_t)mMemPage->getSize())
        {
            destRVA -= (int_t)mMemPage->getBase();

            if(destRVA < selHeadRVA)
            {
                if(rva == destRVA)
                    wPict = GD_HeadFromBottom;
                else if(rva > destRVA && rva < selHeadRVA)
                    wPict = GD_Vert;
                else if(rva == selHeadRVA)
                    wPict = GD_FootToTop;
            }
            else if(destRVA > selHeadRVA)
            {
                if(rva == selHeadRVA)
                    wPict = GD_FootToBottom;
                else if(rva > selHeadRVA && rva < destRVA)
                    wPict = GD_Vert;
                else if(rva == destRVA)
                    wPict = GD_HeadFromTop;
            }
        }
    }

    bool bIsExecute = DbgIsJumpGoingToExecute(rvaToVa(instruction.rva));

    if(branchType == JmpType) //unconditional
    {
        painter->setPen(ConfigColor("DisassemblyUnconditionalJumpLineColor"));
    }
    else
    {
        if(bIsExecute)
            painter->setPen(ConfigColor("DisassemblyConditionalJumpLineTrueColor"));
        else
            painter->setPen(ConfigColor("DisassemblyConditionalJumpLineFalseColor"));
    }

    if(wPict == GD_Vert)
    {
        painter->drawLine(x, y, x, y + getRowHeight());
    }
    else if(wPict == GD_FootToBottom)
    {
        painter->drawLine(x, y + getRowHeight() / 2, x + 5, y + getRowHeight() / 2);
        painter->drawLine(x, y + getRowHeight() / 2, x, y + getRowHeight());
    }
    else if(wPict == GD_FootToTop)
    {
        painter->drawLine(x, y + getRowHeight() / 2, x + 5, y + getRowHeight() / 2);
        painter->drawLine(x, y, x, y + getRowHeight() / 2);
    }
    else if(wPict == GD_HeadFromBottom)
    {
        QPoint wPoints[] =
        {
            QPoint(x + 3, y + getRowHeight() / 2 - 2),
            QPoint(x + 5, y + getRowHeight() / 2),
            QPoint(x + 3, y + getRowHeight() / 2 + 2),
        };

        painter->drawLine(x, y + getRowHeight() / 2, x + 5, y + getRowHeight() / 2);
        painter->drawLine(x, y + getRowHeight() / 2, x, y + getRowHeight());
        painter->drawPolyline(wPoints, 3);
    }
    else if(wPict == GD_HeadFromTop)
    {
        QPoint wPoints[] =
        {
            QPoint(x + 3, y + getRowHeight() / 2 - 2),
            QPoint(x + 5, y + getRowHeight() / 2),
            QPoint(x + 3, y + getRowHeight() / 2 + 2),
        };

        painter->drawLine(x, y + getRowHeight() / 2, x + 5, y + getRowHeight() / 2);
        painter->drawLine(x, y, x, y + getRowHeight() / 2);
        painter->drawPolyline(wPoints, 3);
    }

    return 7;
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
    painter->setPen(QPen(Qt::black, 2)); //thick black line
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
int_t Disassembly::getPreviousInstructionRVA(int_t rva, uint_t count)
{
    QByteArray wBuffer;
    int_t wBottomByteRealRVA;
    int_t wVirtualRVA;
    int_t wMaxByteCountToRead ;

    wBottomByteRealRVA = (int_t)rva - 16 * (count + 3);
    wBottomByteRealRVA = wBottomByteRealRVA < 0 ? 0 : wBottomByteRealRVA;

    wVirtualRVA = (int_t)rva - wBottomByteRealRVA;

    wMaxByteCountToRead = wVirtualRVA + 1 + 16;
    wBuffer.resize(wMaxByteCountToRead);

    mMemPage->read(reinterpret_cast<byte_t*>(wBuffer.data()), wBottomByteRealRVA, wMaxByteCountToRead);

    int_t addr = mDisasm->DisassembleBack(reinterpret_cast<byte_t*>(wBuffer.data()), 0,  wMaxByteCountToRead, wVirtualRVA, count);

    addr += rva - wVirtualRVA;

    return addr;
}


/**
 * @brief       Returns the RVA of count-th instructions after the given instruction RVA.
 *
 * @param[in]   rva         Instruction RVA
 * @param[in]   count       Instruction count
 *
 * @return      RVA of count-th instructions after the given instruction RVA.
 */
int_t Disassembly::getNextInstructionRVA(int_t rva, uint_t count)
{
    QByteArray wBuffer;
    int_t wVirtualRVA = 0;
    int_t wRemainingBytes;
    int_t wMaxByteCountToRead;
    int_t wNewRVA;

    if(mMemPage->getSize() < (uint_t)rva)
        return rva;
    wRemainingBytes = mMemPage->getSize() - rva;

    wMaxByteCountToRead = 16 * (count + 1);
    wMaxByteCountToRead = wRemainingBytes > wMaxByteCountToRead ? wMaxByteCountToRead : wRemainingBytes;
    wBuffer.resize(wMaxByteCountToRead);

    mMemPage->read(reinterpret_cast<byte_t*>(wBuffer.data()), rva, wMaxByteCountToRead);

    wNewRVA = mDisasm->DisassembleNext(reinterpret_cast<byte_t*>(wBuffer.data()), 0,  wMaxByteCountToRead, wVirtualRVA, count);
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
int_t Disassembly::getInstructionRVA(int_t index, int_t count)
{
    int_t wAddr = 0;

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
Instruction_t Disassembly::DisassembleAt(int_t rva)
{
    QByteArray wBuffer;
    int_t base = mMemPage->getBase();
    int_t wMaxByteCountToRead = 16 * 2;

    // Bounding
    //TODO: fix problems with negative sizes
    int_t size = getSize();
    if(!size)
        size = rva;

    wMaxByteCountToRead = wMaxByteCountToRead > (size - rva) ? (size - rva) : wMaxByteCountToRead;

    wBuffer.resize(wMaxByteCountToRead);

    mMemPage->read(reinterpret_cast<byte_t*>(wBuffer.data()), rva, wMaxByteCountToRead);

    return mDisasm->DisassembleAt(reinterpret_cast<byte_t*>(wBuffer.data()), wMaxByteCountToRead, 0, base, rva);
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
Instruction_t Disassembly::DisassembleAt(int_t rva, int_t count)
{
    rva = getNextInstructionRVA(rva, count);
    return DisassembleAt(rva);
}


/************************************************************************************
                                Selection Management
************************************************************************************/
void Disassembly::expandSelectionUpTo(int_t to)
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

void Disassembly::setSingleSelection(int_t index)
{
    mSelection.firstSelectedIndex = index;
    mSelection.fromIndex = index;
    mSelection.toIndex = index;
    emit selectionChanged(rvaToVa(index));
}


int_t Disassembly::getInitialSelection()
{
    return mSelection.firstSelectedIndex;
}

int_t Disassembly::getSelectionSize()
{
    return mSelection.toIndex - mSelection.fromIndex;
}

int_t Disassembly::getSelectionStart()
{
    return mSelection.fromIndex;
}

int_t Disassembly::getSelectionEnd()
{
    return mSelection.toIndex;
}

void Disassembly::selectNext(bool expand)
{
    int_t wAddr;
    int_t wStart = getInstructionRVA(getSelectionStart(), 1) - 1;
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
            int_t wInstrSize = getInstructionRVA(wAddr, 1) - wAddr - 1;
            expandSelectionUpTo(wAddr + wInstrSize);
        }
    }
    else //select next instruction
    {
        wAddr = getSelectionEnd() + 1;
        setSingleSelection(wAddr);
        int_t wInstrSize = getInstructionRVA(wAddr, 1) - wAddr - 1;
        expandSelectionUpTo(wAddr + wInstrSize);
    }
}


void Disassembly::selectPrevious(bool expand)
{
    int_t wAddr;
    int_t wStart = getInstructionRVA(getSelectionStart(), 1) - 1;
    if(expand)
    {
        if(getSelectionStart() == getInitialSelection() && wStart != getSelectionEnd()) //decrease up
        {
            wAddr = getInstructionRVA(getSelectionEnd() + 1, -2);
            int_t wInstrSize = getInstructionRVA(wAddr, 1) - wAddr - 1;
            expandSelectionUpTo(wAddr + wInstrSize);
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
        int_t wInstrSize = getInstructionRVA(wAddr, 1) - wAddr - 1;
        expandSelectionUpTo(wAddr + wInstrSize);
    }
}


bool Disassembly::isSelected(int_t base, int_t offset)
{
    int_t wAddr = base;

    if(offset < 0)
        wAddr = getPreviousInstructionRVA(getTableOffset(), offset);
    else if(offset > 0)
        wAddr = getNextInstructionRVA(getTableOffset(), offset);

    if(wAddr >= mSelection.fromIndex && wAddr <= mSelection.toIndex)
        return true;
    else
        return false;
}


bool Disassembly::isSelected(QList<Instruction_t>* buffer, int index)
{
    if(buffer->size() > 0 && index >= 0 && index < buffer->size())
    {
        if((int_t)buffer->at(index).rva >= mSelection.fromIndex && (int_t)buffer->at(index).rva <= mSelection.toIndex)
            return true;
        else
            return false;
    }
    else
    {
        return false;
    }
}


/************************************************************************************
                         Update/Reload/Refresh/Repaint
************************************************************************************/

void Disassembly::prepareDataCount(int_t wRVA, int wCount, QList<Instruction_t>* instBuffer)
{
    instBuffer->clear();
    Instruction_t wInst;
    for(int wI = 0; wI < wCount; wI++)
    {
        wInst = DisassembleAt(wRVA);
        instBuffer->append(wInst);
        wRVA += wInst.length;
    }
}

void Disassembly::prepareDataRange(int_t startRva, int_t endRva, QList<Instruction_t>* instBuffer)
{
    if(startRva == endRva)
        prepareDataCount(startRva, 1, instBuffer);
    else
    {
        int wCount = 0;
        int_t addr = startRva;
        while(addr <= endRva)
        {
            addr = getNextInstructionRVA(addr, 1);
            wCount++;
        }
        prepareDataCount(startRva, wCount, instBuffer);
    }
}

void Disassembly::prepareData()
{
    int_t wViewableRowsCount = getViewableRowsCount();

    int_t wAddrPrev = getTableOffset();
    int_t wAddr = wAddrPrev;

    int wCount = 0;

    for(int wI = 0; wI < wViewableRowsCount && getRowCount() > 0; wI++)
    {
        wAddrPrev = wAddr;
        wAddr = getNextInstructionRVA(wAddr, 1);

        if(wAddr == wAddrPrev)
        {
            break;
        }

        wCount++;
    }

    setNbrOfLineToPrint(wCount);

    prepareDataCount(getTableOffset(), wCount, &mInstBuffer);
}

void Disassembly::reloadData()
{
    emit selectionChanged(rvaToVa(mSelection.firstSelectedIndex));
    AbstractTableView::reloadData();
}


/************************************************************************************
                        Public Methods
************************************************************************************/
uint_t Disassembly::rvaToVa(int_t rva)
{
    return mMemPage->va(rva);
}

void Disassembly::disassembleAt(int_t parVA, int_t parCIP, bool history, int_t newTableOffset)
{
    int_t wBase = DbgMemFindBaseAddr(parVA, 0);
    int_t wSize = DbgMemGetPageSize(wBase);
    if(!wBase || !wSize)
        return;
    int_t wRVA = parVA - wBase;
    int_t wCipRva = parCIP - wBase;

    HistoryData_t newHistory;

    //VA history
    if(history)
    {
        //truncate everything right from the current VA
        if(mVaHistory.size() && mCurrentVa < mVaHistory.size() - 1) //mCurrentVa is not the last
            mVaHistory.erase(mVaHistory.begin() + mCurrentVa + 1, mVaHistory.end());

        //NOTE: mCurrentVa always points to the last entry of the list

        //add the currently selected address to the history
        int_t selectionVA = rvaToVa(getInitialSelection()); //currently selected VA
        int_t selectionTableOffset = getTableOffset();
        if(selectionVA && mVaHistory.size() && mVaHistory.last().va != selectionVA) //do not have 2x the same va in a row
        {
            if(mVaHistory.size() >= 1024) //max 1024 in the history
            {
                mCurrentVa--;
                mVaHistory.erase(mVaHistory.begin()); //remove the oldest element
            }
            mCurrentVa++;
            newHistory.va = selectionVA;
            newHistory.tableOffset = selectionTableOffset;
            mVaHistory.push_back(newHistory);
        }
    }

    // Set base and size (Useful when memory page changed)
    mMemPage->setAttributes(wBase, wSize);

    if(mRvaDisplayEnabled && mMemPage->getBase() != mRvaDisplayPageBase)
        mRvaDisplayEnabled = false;

    setRowCount(wSize);

    setSingleSelection(wRVA);               // Selects disassembled instruction
    int_t wInstrSize = getInstructionRVA(wRVA, 1) - wRVA - 1;
    expandSelectionUpTo(wRVA + wInstrSize);

    //set CIP rva
    mCipRva = wCipRva;

    if(newTableOffset == -1) //nothing specified
    {
        // Update table offset depending on the location of the instruction to disassemble
        if(mInstBuffer.size() > 0 && wRVA >= (int_t)mInstBuffer.first().rva && wRVA < (int_t)mInstBuffer.last().rva)
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
                repaint();
            }
            else
            {
                setTableOffset(wRVA);
            }
        }
        else if(mInstBuffer.size() > 0 && wRVA == (int_t)mInstBuffer.last().rva)
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
    emit disassembledAt(parVA,  parCIP,  history,  newTableOffset);
    reloadData();

}

QList<Instruction_t>* Disassembly::instructionsBuffer()
{
    return &mInstBuffer;
}

const int_t Disassembly::currentEIP() const
{
    return mCipRva;
}


void Disassembly::disassembleAt(int_t parVA, int_t parCIP)
{
    disassembleAt(parVA, parCIP, true, -1);
}


void Disassembly::disassembleClear()
{
    mHighlightingMode = false;
    mHighlightToken.value.value = 0;
    mHighlightToken.text = "";
    historyClear();
    mMemPage->setAttributes(0, 0);
    setRowCount(0);
    reloadData();
}


void Disassembly::debugStateChangedSlot(DBGSTATE state)
{
    switch(state)
    {
    case stopped:
        disassembleClear();
        break;
    case paused:
        mIsRunning = false;
        break;
    case running:
        mIsRunning = true;
        break;
    default:
        break;
    }
}

const int_t Disassembly::getBase() const
{
    return mMemPage->getBase();
}

int_t Disassembly::getSize()
{
    return mMemPage->getSize();
}

void Disassembly::historyClear()
{
    mVaHistory.clear(); //clear history for new targets
    mCurrentVa = 0;
}

void Disassembly::historyPrevious()
{
    if(!mCurrentVa || !mVaHistory.size()) //we are at the earliest history entry
        return;
    mCurrentVa--;
    disassembleAt(mVaHistory.at(mCurrentVa).va, rvaToVa(mCipRva), false, mVaHistory.at(mCurrentVa).tableOffset);
}

void Disassembly::historyNext()
{
    int size = mVaHistory.size();
    if(!size || mCurrentVa >= mVaHistory.size() - 1) //we are at the newest history entry
        return;
    mCurrentVa++;
    disassembleAt(mVaHistory.at(mCurrentVa).va, rvaToVa(mCipRva), false, mVaHistory.at(mCurrentVa).tableOffset);
}

bool Disassembly::historyHasPrevious()
{
    if(!mCurrentVa || !mVaHistory.size()) //we are at the earliest history entry
        return false;
    return true;
}

bool Disassembly::historyHasNext()
{
    int size = mVaHistory.size();
    if(!size || mCurrentVa >= mVaHistory.size() - 1) //we are at the newest history entry
        return false;
    return true;
}

QString Disassembly::getAddrText(int_t cur_addr, char label[MAX_LABEL_SIZE])
{
    QString addrText = "";
    if(mRvaDisplayEnabled) //RVA display
    {
        int_t rva = cur_addr - mRvaDisplayBase;
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
    addrText += QString("%1").arg(cur_addr, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    char label_[MAX_LABEL_SIZE] = "";
    if(DbgGetLabelAt(cur_addr, SEG_DEFAULT, label_)) //has label
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
