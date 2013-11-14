#include "HexDump.h"



HexDump::HexDump(QWidget *parent) :AbstractTableView(parent)
{
    SelectionData_t data;
    memset(&data, 0, sizeof(SelectionData_t));
    mSelection = data;

    mGuiState = HexDump::NoState;


    mByteWidth = QFontMetrics(this->font()).width(QChar('L')) + 4;
    mDumpByteWidth = 16;

    setRowCount(0);

    mMemPage = new MemoryPage(0, 0);

    addColumnAt(100, false);
    addColumnAt(100, false);
    addColumnAt(100, false);

    connect(Bridge::getBridge(), SIGNAL(disassembleAt(int_t, int_t)), this, SLOT(printDumpAt(int_t)));
}


void HexDump::printDumpAt(int_t parVA)
{
    int_t wBase = Bridge::getBridge()->getBase(parVA);
    int_t wSize = Bridge::getBridge()->getSize(wBase);
    int_t wRVA = parVA - wBase;

    setRowCount(wSize/mDumpByteWidth);
    mMemPage->setAttributes(wBase, wSize);  // Set base and size (Useful when memory page changed)
    setTableOffset(wRVA/mDumpByteWidth);
}

void HexDump::mouseMoveEvent(QMouseEvent* event)
{
    qDebug() << "HexDump::mouseMoveEvent";

    bool wAccept = true;

    if(mGuiState == HexDump::MultiRowsSelectionState)
    {
        qDebug() << "State = MultiRowsSelectionState";

        if((transY(event->y()) >= 0) && (transY(event->y()) <= this->getTableHeigth()))
        {
            int wRowIndex = getTableOffset() +getIndexOffsetFromY(transY(event->y()));

            if(wRowIndex < getRowCount())
            {
                expandSelectionUpTo(wRowIndex);

                this->viewport()->repaint();

                wAccept = false;
            }
        }
    }

    if(wAccept == true)
        AbstractTableView::mouseMoveEvent(event);
}



void HexDump::mousePressEvent(QMouseEvent* event)
{
    qDebug() << "HexDump::mousePressEvent";

    bool wAccept = false;

    if(((event->buttons() & Qt::LeftButton) != 0) && ((event->buttons() & Qt::RightButton) == 0))
    {
        if(getGuiState() == AbstractTableView::NoState)
        {
            if(event->y() > getHeaderHeigth())
            {
                int wRowIndex = getTableOffset()+ getIndexOffsetFromY(transY(event->y()));
                int wColIndex = getColumnIndexFromX(event->x());
                int wByteIndex;

                if(wRowIndex < getRowCount())
                {
                    if(wColIndex == 1)
                    {
                        int wX = event->x() - getColumnPosition(wColIndex);

                    }
                    else if(wColIndex == 2)
                    {
                        int wColBegin = getColumnPosition(wColIndex);

                    }

                    setSingleSelection(wRowIndex);

                    mGuiState = HexDump::MultiRowsSelectionState;

                    viewport()->repaint();

                    wAccept = true;
                }
            }
        }
    }

    if(wAccept == false)
        AbstractTableView::mousePressEvent(event);
}



void HexDump::mouseReleaseEvent(QMouseEvent* event)
{
    bool wAccept = true;

    if((event->buttons() & Qt::LeftButton) == 0)
    {
        if(mGuiState == HexDump::MultiRowsSelectionState)
        {
            mGuiState = HexDump::NoState;

            this->viewport()->repaint();

            wAccept = false;
        }
    }

    if(wAccept == true)
        AbstractTableView::mouseReleaseEvent(event);
}





QString HexDump::paintContent(QPainter* painter, int_t rowBase, int rowOffset, int col, int x, int y, int w, int h)
{
    //return QString("HexDump: Col:") + QString::number(col) + "Row:" + QString::number(rowBase + rowOffset);

    QString wStr = "";
    int_t wRva = (rowBase + rowOffset) * mDumpByteWidth;


    //if(isSelected(rowBase, rowOffset) == true)
    //    painter->fillRect(QRect(x, y, w, h), QBrush(QColor(192,192,192)));

    switch(col)
    {
        case 0:
        {
            //uint_t wAddr = (uint_t)instruction.rva + (uint_t)mMemoryView->getBase();
            //wStr = QString("%1").arg(wAddr, 8, 16, QChar('0')).toUpper();
            wStr += QString::number(wRva);
            break;
        }

        case 1:
        {
            QByteArray wBuffer;
            wBuffer.resize(mDumpByteWidth);
            mMemPage->readOriginalMemory(reinterpret_cast<byte_t*>(wBuffer.data()), wRva, mDumpByteWidth);

            for(int i = 0; i < mDumpByteWidth; i++)
                wStr += QString("%1").arg((unsigned char)wBuffer.at(i), 2, 16, QChar('0')).toUpper() + " ";

            break;
        }

        case 2:
        {
            wStr = "ToDo";
            break;
        }

        default:
            break;
    }

    return wStr;
}




/************************************************************************************
                                Selection Management
************************************************************************************/
void HexDump::expandSelectionUpTo(int to)
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


void HexDump::setSingleSelection(int index)
{
    mSelection.firstSelectedIndex = index;
    mSelection.fromIndex = index;
    mSelection.toIndex = index;
}


int HexDump::getInitialSelection()
{
    return mSelection.firstSelectedIndex;
}


bool HexDump::isSelected(int base, int offset)
{
    /*
    int wAddr = base;

    if(offset < 0)
        wAddr = getPreviousInstructionRVA(getTableOffset(), offset);
    else if(offset > 0)
        wAddr = getNextInstructionRVA(getTableOffset(), offset);

    if(wAddr >= mSelection.fromIndex && wAddr <= mSelection.toIndex)
        return true;
    else
        return false;
        */
    return false;
}


