#include "Disassembly.h"

Disassembly::Disassembly(MemoryPage* parMemPage, QWidget *parent) : AbstractTableView(parent)
{
    setMemoryPage(parMemPage);

    mInstBuffer.clear();

    SelectionData_t data;
    memset(&data, 0, sizeof(SelectionData_t));
    mSelection = data;

    mCipRva = 0;

    mDisasm = new QBeaEngine();

    mIsLastInstDisplayed = false;

    mGuiState = Disassembly::NoState;

    setRowCount(parMemPage->getSize());

    qDebug() << "size" << parMemPage->getSize();

    int charwidth=QFontMetrics(this->font()).width(QChar(' '));

    addColumnAt(charwidth*2*sizeof(int_t)+8, false); //address
    addColumnAt(charwidth*2*12+8, false); //bytes
    addColumnAt(charwidth*40, false); //disassembly
    addColumnAt(100, false); //comments

    connect(Bridge::getBridge(), SIGNAL(disassembleAt(int_t, int_t)), this, SLOT(disassambleAt(int_t, int_t)));
    connect(Bridge::getBridge(), SIGNAL(dbgStateChanged(DBGSTATE)), this, SLOT(debugStateChangedSlot(DBGSTATE)));
}

/************************************************************************************
                            Private Functions
************************************************************************************/

void Disassembly::paintRichText(QPainter* painter, int x, int y, int w, int h, int xinc, const QList<CustomRichText_t>* richText)
{
    int len=richText->size();
    int charwidth=QFontMetrics(this->font()).width(QChar(' '));
    for(int i=0; i<len; i++)
    {
        CustomRichText_t curRichText=richText->at(i);
        int curRichTextLength=curRichText.text.length();
        int backgroundWidth=charwidth*curRichTextLength;
        if(backgroundWidth+xinc>w)
            backgroundWidth=w-xinc;
        switch(curRichText.flags)
        {
        case FlagNone: //defaults
            painter->drawText(QRect(x+xinc, y, w-xinc, h), 0, curRichText.text);
            break;
        case FlagColor: //color only
            painter->save();
            painter->setPen(QPen(curRichText.textColor));
            painter->drawText(QRect(x+xinc, y, w-xinc, h), 0, curRichText.text);
            painter->restore();
            break;
        case FlagBackground: //background only
            painter->save();
            if(backgroundWidth>0)
                painter->fillRect(QRect(x+xinc, y, backgroundWidth, h), QBrush(curRichText.textBackground));
            painter->drawText(QRect(x+xinc, y, w-xinc, h), 0, curRichText.text);
            painter->restore();
            break;
        case FlagAll: //color+background
            painter->save();
            if(backgroundWidth>0)
                painter->fillRect(QRect(x+xinc, y, backgroundWidth, h), QBrush(curRichText.textBackground));
            painter->setPen(QPen(curRichText.textColor));
            painter->drawText(QRect(x+xinc, y, w-xinc, h), 0, curRichText.text);
            painter->restore();
            break;
        }
        xinc+=charwidth*curRichTextLength;
    }
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
    QString wStr = "";
    int_t wRVA = mInstBuffer.at(rowOffset).rva;
    bool wIsSelected = isSelected(rowBase, rowOffset);

    // Highlight if selected
    if(wIsSelected)
        painter->fillRect(QRect(x, y, w, h), QBrush(QColor(192,192,192)));

    switch(col)
    {
    case 0: // Draw address (+ label)
    {
        char label[MAX_LABEL_SIZE]="";
        int_t cur_addr=mInstBuffer.at(rowOffset).rva+mMemPage->getBase();
        QString addrText=QString("%1").arg(cur_addr, sizeof(int_t)*2, 16, QChar('0')).toUpper();
        if(DbgGetLabelAt(cur_addr, SEG_DEFAULT, label)) //has label
            addrText+=" <"+QString(label)+">";
        else
            *label=0;
        BPXTYPE bpxtype=DbgGetBpxTypeAt(cur_addr);
        painter->save();
        if(mInstBuffer.at(rowOffset).rva == mCipRva) //cip
        {
            painter->fillRect(QRect(x, y, w, h), QBrush(QColor(0,0,0)));
            switch(bpxtype) //breakpoint
            {
            case bpnormal:
                painter->setPen(QPen(QColor("#ff0000")));
                break;
            default:
                painter->setPen(QPen(QColor("#fffbf0")));
                break;
            }
        }
        else //other address
        {
            if(*label) //label
            {
                if(bpxtype==bpnone) //label only
                    painter->setPen(QPen(QColor("#ff0000"))); //red -> address + label text
                else //label+breakpoint
                {
                    switch(bpxtype)
                    {
                    case bpnormal:
                        painter->fillRect(QRect(x, y, w, h), QBrush(QColor("#ff0000"))); //fill red
                        break;
                    default:
                        painter->setPen(QPen(QColor("#000000"))); //black address
                        break;
                    }                    
                }
            }
            else //no label
            {
                if(bpxtype==bpnone) //no label, no breakpoint
                {
                    if(wIsSelected)
                        painter->setPen(QPen(QColor("#000000"))); //black address
                    else
                        painter->setPen(QPen(QColor("#808080")));
                }
                else //breakpoint only
                {
                    switch(bpxtype)
                    {
                    case bpnormal:
                        painter->fillRect(QRect(x, y, w, h), QBrush(QColor("#ff0000"))); //fill red
                        break;
                    default:
                        if(wIsSelected)
                            painter->setPen(QPen(QColor("#000000"))); //black address
                        else
                            painter->setPen(QPen(QColor("#808080")));
                        break;
                    }
                }
            }
        }
        painter->drawText(QRect(x + 4, y , w - 4 , h), Qt::AlignVCenter | Qt::AlignLeft, addrText);
        painter->restore();
        break;
    }

    case 1: //draw bytes (TODO: some spaces between bytes)
    {
        for(int i = 0; i < mInstBuffer.at(rowOffset).dump.size(); i++)
            wStr += QString("%1").arg((unsigned char)(mInstBuffer.at(rowOffset).dump.at(i)), 2, 16, QChar('0')).toUpper();

        paintJumpsGraphic(painter, x + 5, y, wRVA);

        // Draw cell content
        painter->drawText(QRect(x + 15, y, getColumnWidth(col) - 15, getRowHeight()), 0, wStr);

        wStr = "";

        break;
    }

    case 2: //draw disassembly (with colours needed)
    {
        QList<CustomRichText_t> richText;
        BeaHighlight::PrintRtfInstruction(&richText, &mInstBuffer.at(rowOffset).disasm);
        Disassembly::paintRichText(painter, x, y, getColumnWidth(col), getRowHeight(), 4, &richText);
        break;
    }

    case 3: //draw comments
    {
        char comment[MAX_COMMENT_SIZE]="";
        if(DbgGetCommentAt(mInstBuffer.at(rowOffset).rva+mMemPage->getBase(), comment))
            wStr=QString(comment);
        else
            wStr="";
    }
        break;

    default:
        break;
    }

    return wStr;
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

            // Bound
            wI = wI >= mInstBuffer.size() ? mInstBuffer.size() - 1 : wI;
            wI = wI < 0 ? 0 : wI;

            int_t wRowIndex = mInstBuffer.at(wI).rva;

            if(wRowIndex < getRowCount())
            {
                expandSelectionUpTo(wRowIndex);

                repaint();

                wAccept = false;
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
    //qDebug() << "Disassembly::mousePressEvent";

    bool wAccept = false;

    if(((event->buttons() & Qt::LeftButton) != 0) && ((event->buttons() & Qt::RightButton) == 0))
    {
        if(getGuiState() == AbstractTableView::NoState)
        {
            if(event->y() > getHeaderHeigth())
            {
                int_t wRowIndex = getInstructionRVA(getTableOffset(), getIndexOffsetFromY(transY(event->y())));

                if(wRowIndex < getRowCount())
                {
                    setSingleSelection(wRowIndex);

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

        if(key == Qt::Key_Up)
            selectPrevious();
        else
            selectNext();

        if(getInitialSelection() < botRVA)
        {
            setTableOffset(getInitialSelection());
        }
        else if(getInitialSelection() >= topRVA)
        {
            setTableOffset(getInstructionRVA(getInitialSelection(),-getNbrOfLineToPrint() + 2));
        }

        repaint();
    }
    else
    {
        AbstractTableView::keyPressEvent(event);
    }
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
void Disassembly::paintJumpsGraphic(QPainter* painter, int x, int y, int_t addr)
{
    int_t selHeadRVA = mSelection.fromIndex;
    int_t rva = addr;
    Instruction_t instruction = DisassembleAt(selHeadRVA);
    Int32 branchType = instruction.disasm.Instruction.BranchType;
    GraphicDump_t wPict = GD_Nothing;

    if(     branchType == (Int32)JO      ||
            branchType == (Int32)JC      ||
            branchType == (Int32)JE      ||
            branchType == (Int32)JA      ||
            branchType == (Int32)JS      ||
            branchType == (Int32)JP      ||
            branchType == (Int32)JL      ||
            branchType == (Int32)JG      ||
            branchType == (Int32)JB      ||
            branchType == (Int32)JECXZ   ||
            branchType == (Int32)JmpType ||
            branchType == (Int32)RetType ||
            branchType == (Int32)JNO     ||
            branchType == (Int32)JNC     ||
            branchType == (Int32)JNE     ||
            branchType == (Int32)JNA     ||
            branchType == (Int32)JNS     ||
            branchType == (Int32)JNP     ||
            branchType == (Int32)JNL     ||
            branchType == (Int32)JNG     ||
            branchType == (Int32)JNB)
    {
        int_t destRVA = (int_t)instruction.disasm.Instruction.AddrValue;

        if(destRVA > (int_t)mMemPage->getBase())
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

    painter->save() ;

    if(DbgIsJumpGoingToExecute(instruction.rva+mMemPage->getBase())) //change pen color when jump is executed
        painter->setPen(QColor(255, 0, 0));
    else
        painter->setPen(QColor(128, 128, 128));

    if(wPict == GD_Vert)
    {
        painter->drawLine(x, y, x, y + getRowHeight());
    }
    else if(wPict == GD_FootToBottom)
    {
        painter->drawLine(x, y + getRowHeight() / 2, x + 5, y + getRowHeight() / 2);
        painter->drawLine(x, y + getRowHeight() / 2, x, y + getRowHeight());
    }
    if(wPict == GD_FootToTop)
    {
        painter->drawLine(x, y + getRowHeight() / 2, x + 5, y + getRowHeight() / 2);
        painter->drawLine(x, y, x, y + getRowHeight() / 2);
    }
    else if(wPict == GD_HeadFromBottom)
    {
        QPoint wPoints[] = {
            QPoint(x + 3, y + getRowHeight() / 2 - 2),
            QPoint(x + 5, y + getRowHeight() / 2),
            QPoint(x + 3, y + getRowHeight() / 2 + 2),
        };

        painter->drawLine(x, y + getRowHeight() / 2, x + 5, y + getRowHeight() / 2);
        painter->drawLine(x, y + getRowHeight() / 2, x, y + getRowHeight());
        painter->drawPolyline(wPoints, 3);
    }
    if(wPict == GD_HeadFromTop)
    {
        QPoint wPoints[] = {
            QPoint(x + 3, y + getRowHeight() / 2 - 2),
            QPoint(x + 5, y + getRowHeight() / 2),
            QPoint(x + 3, y + getRowHeight() / 2 + 2),
        };

        painter->drawLine(x, y + getRowHeight() / 2, x + 5, y + getRowHeight() / 2);
        painter->drawLine(x, y, x, y + getRowHeight() / 2);
        painter->drawPolyline(wPoints, 3);
    }

    painter->restore();
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

    mMemPage->readOriginalMemory(reinterpret_cast<byte_t*>(wBuffer.data()), wBottomByteRealRVA, wMaxByteCountToRead);

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

    wRemainingBytes = mMemPage->getSize() - rva;

    wMaxByteCountToRead = 16 * (count + 1);
    wMaxByteCountToRead = wRemainingBytes > wMaxByteCountToRead ? wMaxByteCountToRead : wRemainingBytes;
    wBuffer.resize(wMaxByteCountToRead);

    mMemPage->readOriginalMemory(reinterpret_cast<byte_t*>(wBuffer.data()), rva, wMaxByteCountToRead);

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
    wBuffer.resize(wMaxByteCountToRead);

    mMemPage->readOriginalMemory(reinterpret_cast<byte_t*>(wBuffer.data()), rva, wMaxByteCountToRead);

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
        mSelection.toIndex = mSelection.firstSelectedIndex;
    }
    else if(to > mSelection.firstSelectedIndex)
    {
        mSelection.fromIndex = mSelection.firstSelectedIndex;
        mSelection.toIndex = to;
    }
}


void Disassembly::setSingleSelection(int_t index)
{
    mSelection.firstSelectedIndex = index;
    mSelection.fromIndex = index;
    mSelection.toIndex = index;
}


int_t Disassembly::getInitialSelection()
{
    return mSelection.firstSelectedIndex;
}


void Disassembly::selectNext()
{
    int_t wAddr = getInstructionRVA(getInitialSelection(), 1);

    setSingleSelection(wAddr);
}


void Disassembly::selectPrevious()
{
    int_t wAddr = getInstructionRVA(getInitialSelection(), -1);

    setSingleSelection(wAddr);
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


/************************************************************************************
                         Update/Reload/Refresh/Repaint
************************************************************************************/
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

    int wI = 0;
    int_t wRVA = 0;
    Instruction_t wInst;

    wRVA = getTableOffset();
    mInstBuffer.clear();

    for(wI = 0; wI < wCount; wI++)
    {
        wInst = DisassembleAt(wRVA);
        mInstBuffer.append(wInst);
        wRVA += wInst.lentgh;
    }
}


/************************************************************************************
                        Public Methods
************************************************************************************/
void Disassembly::setMemoryPage(MemoryPage* parMemPage)
{
    mMemPage = parMemPage;
}


void Disassembly::disassambleAt(int_t parVA, int_t parCIP)
{
    int_t wBase = Bridge::getBridge()->getBase(parVA);
    int_t wSize = Bridge::getBridge()->getSize(wBase);
    int_t wRVA = parVA - wBase;
    int_t wCipRva = parCIP - wBase;

    setRowCount(wSize);
    mMemPage->setAttributes(wBase, wSize);  // Set base and size (Useful when memory page changed)
    setSingleSelection(wRVA);               // Selects disassembled instruction

    mCipRva = wCipRva;

    if(mInstBuffer.size() > 0 && wRVA >= (int_t)mInstBuffer.first().rva && wRVA < (int_t)mInstBuffer.last().rva)
    {
        repaint();
    }
    else if(mInstBuffer.size() > 0 && wRVA == (int_t)mInstBuffer.last().rva)
    {
        setTableOffset(mInstBuffer.first().rva + mInstBuffer.first().lentgh);
    }
    else
    {
        setTableOffset(wRVA);
    }

}


void Disassembly::disassembleClear()
{
    //TODO: fix this (also try restarting)
    setRowCount(0);
    reloadData();
}


void Disassembly::debugStateChangedSlot(DBGSTATE state)
{
    if(state==stopped)
        disassembleClear();
}
