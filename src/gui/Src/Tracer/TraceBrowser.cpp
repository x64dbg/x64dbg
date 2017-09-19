#include "TraceBrowser.h"
#include "TraceFileReader.h"
#include "RichTextPainter.h"
#include "BrowseDialog.h"
#include "QBeaEngine.h"
#include "GotoDialog.h"

TraceBrowser::TraceBrowser(QWidget* parent) : AbstractTableView(parent)
{
    mTraceFile = nullptr;
    addColumnAt(getCharWidth() * 2 * 8 + 8, "", false); //index
    addColumnAt(getCharWidth() * 2 * sizeof(dsint) + 8, "", false); //address
    addColumnAt(getCharWidth() * 2 * 12 + 8, "", false); //bytes
    addColumnAt(getCharWidth() * 40, "", false); //disassembly
    addColumnAt(1000, "", false); //comments

    setShowHeader(false); //hide header

    mSelection.firstSelectedIndex = 0;
    mSelection.fromIndex = 0;
    mSelection.toIndex = 0;
    setRowCount(0);

    int maxModuleSize = (int)ConfigUint("Disassembler", "MaxModuleSize");
    mDisasm = new QBeaEngine(maxModuleSize);

    setupRightClickContextMenu();

    Initialize();
}

TraceBrowser::~TraceBrowser()
{
    delete mDisasm;
}

QString TraceBrowser::paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h)
{
    int index = rowBase + rowOffset;
    bool isSelected = (index >= mSelection.fromIndex && index <= mSelection.toIndex);
    if(isSelected)
    {
        painter->fillRect(QRect(x, y, w, h), QBrush(selectionColor));
    }

    if(!mTraceFile || mTraceFile->Progress() != 100 || index >= mTraceFile->Length())
    {
        return "";
    }
    switch(col)
    {
    case 0: //index
    {
        QString indexString;
        indexString = QString::number(index, 16).toUpper();
        int digits = ceil(log2(mTraceFile->Length()) / 4) + 1;
        digits -= indexString.size();
        while(digits > 0)
        {
            indexString = '0' + indexString;
            digits = digits - 1;
        }
        return indexString;
    }

    case 1: //address
    {
        return ToPtrString(mTraceFile->Registers(index).regcontext.cip);
    }

    case 2: //opcode
    {
        RichTextPainter::List richBytes;
        RichTextPainter::CustomRichText_t curByte;
        RichTextPainter::CustomRichText_t space;
        unsigned char opcodes[16];
        int opcodeSize = 0;
        mTraceFile->OpCode(index, opcodes, &opcodeSize);
        space.text = " ";
        space.flags = RichTextPainter::FlagNone;
        space.highlightWidth = 1;
        space.highlightConnectPrev = true;
        curByte.flags = RichTextPainter::FlagAll;
        curByte.highlightWidth = 1;
        space.highlight = false;
        curByte.highlight = false;

        for(int i = 0; i < opcodeSize; i++)
        {
            if(i)
                richBytes.push_back(space);

            curByte.text = ToByteString(opcodes[i]);
            curByte.textColor = mBytesColor;
            curByte.textBackground = mBytesBackgroundColor;
            richBytes.push_back(curByte);
        }

        RichTextPainter::paintRichText(painter, x, y, getColumnWidth(col), getRowHeight(), 4, richBytes, mFontMetrics);
        return "";
    }

    case 3: //disassembly
    {
        RichTextPainter::List richText;
        unsigned char opcodes[16];
        int opcodeSize = 0;
        mTraceFile->OpCode(index, opcodes, &opcodeSize);

        Instruction_t inst = mDisasm->DisassembleAt(opcodes, opcodeSize, 0, mTraceFile->Registers(index).regcontext.cip, false);

        //if(mHighlightToken.text.length())
        //CapstoneTokenizer::TokenToRichText(inst.tokens, richText, &mHighlightToken);
        //else
        CapstoneTokenizer::TokenToRichText(inst.tokens, richText, 0);
        RichTextPainter::paintRichText(painter, x + 0, y, getColumnWidth(col) - 0, getRowHeight(), 4, richText, mFontMetrics);
        return "";
    }

    case 4: //comments
    {
        if(DbgIsDebugging())
        {
            char comments[MAX_COMMENT_SIZE];
            if(DbgGetCommentAt(mTraceFile->Registers(index).regcontext.cip, comments))
            {
                return QString::fromUtf8(comments);
            }
        }
        return "";
    }
    default:
        return "";
    }
}

void TraceBrowser::prepareData()
{
    auto viewables = getViewableRowsCount();
    int lines = 0;
    if(mTraceFile != nullptr)
    {
        if(mTraceFile->Progress() == 100)
        {
            if(mTraceFile->Length() < getTableOffset() + viewables)
                lines = mTraceFile->Length() - getTableOffset();
            else
                lines = viewables;
        }
    }
    setNbrOfLineToPrint(lines);
}

void TraceBrowser::setupRightClickContextMenu()
{
    mMenuBuilder = new MenuBuilder(this);
    mMenuBuilder->addAction(makeAction(DIcon("folder-horizontal-open.png"), tr("Open"), SLOT(openFileSlot())), [this](QMenu*)
    {
        return mTraceFile == nullptr;
    });
    mMenuBuilder->addAction(makeAction(DIcon("fatal-error.png"), tr("Close"), SLOT(closeFileSlot())), [this](QMenu*)
    {
        return mTraceFile != nullptr;
    });
    mMenuBuilder->addSeparator();
    auto isValid = [this](QMenu*)
    {
        return mTraceFile != nullptr && mTraceFile->Progress() == 100;
    };
    MenuBuilder* copyMenu = new MenuBuilder(this, isValid);
    copyMenu->addAction(makeAction(DIcon("copy_disassembly.png"), tr("Disassembly"), SLOT(copyDisassemblySlot())));
    mMenuBuilder->addMenu(makeMenu(DIcon("copy.png"), tr("&Copy")), copyMenu);

    mMenuBuilder->addAction(makeShortcutAction(DIcon("goto.png"), tr("Go to..."), SLOT(gotoSlot()), "ActionGotoExpression"), isValid);
}

void TraceBrowser::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu(this);
    mMenuBuilder->build(&menu);
    menu.exec(event->globalPos());
}

void TraceBrowser::mousePressEvent(QMouseEvent* event)
{
    duint index = getIndexOffsetFromY(transY(event->y())) + getTableOffset();
    switch(event->button())
    {
    case Qt::LeftButton:
        if(index < getRowCount())
        {
            if(event->modifiers() & Qt::ShiftModifier)
                expandSelectionUpTo(index);
            else
                setSingleSelection(index);
            updateViewport();
            return;
        }
    }
    AbstractTableView::mousePressEvent(event);
}

void TraceBrowser::mouseMoveEvent(QMouseEvent* event)
{
    duint index = getIndexOffsetFromY(transY(event->y())) + getTableOffset();
    if(event->buttons() & Qt::LeftButton)
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

void TraceBrowser::mouseReleaseEvent(QMouseEvent* event)
{
    AbstractTableView::mouseReleaseEvent(event);
    updateViewport();
}

void TraceBrowser::keyPressEvent(QKeyEvent* event)
{
    int key = event->key();
    int curindex = getInitialSelection();
    int visibleindex = curindex;
    if((key == Qt::Key_Up || key == Qt::Key_Down) && mTraceFile && mTraceFile->Progress() == 100)
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
            if(getSelectionEnd() + 1 < mTraceFile->Length())
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
        updateViewport();
    }
    else
        AbstractTableView::keyPressEvent(event);
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
    if(index < getTableOffset())
        setTableOffset(index);
    else if(index + 2 > getTableOffset() + getViewableRowsCount())
        setTableOffset(index - getViewableRowsCount() + 2);
}

void TraceBrowser::updateColors()
{
    AbstractTableView::updateColors();
    //CapstoneTokenizer::UpdateColors(); //Already called in disassembly
    mDisasm->UpdateConfig();
    mBytesColor = ConfigColor("DisassemblyBytesColor");
    mBytesBackgroundColor = ConfigColor("DisassemblyBytesBackgroundColor");
}

void TraceBrowser::openFileSlot()
{
    BrowseDialog browse(this, tr("Open run trace file"), tr("Open trace file"), tr("Run trace files (*.%1);;All files (*.*)").arg(ArchValue("trace32", "trace64")), QApplication::applicationDirPath(), false);
    if(browse.exec() != QDialog::Accepted)
        return;
    mTraceFile = new TraceFileReader(this);
    connect(mTraceFile, SIGNAL(parseFinished()), this, SLOT(parseFinishedSlot()));
    mTraceFile->Open(browse.path);
}

void TraceBrowser::closeFileSlot()
{
    mTraceFile->Close();
    delete mTraceFile;
    mTraceFile = nullptr;
    reloadData();
}

void TraceBrowser::parseFinishedSlot()
{
    if(mTraceFile->isError())
    {
        SimpleErrorBox(this, tr("Error"), "Error when opening run trace file");
        delete mTraceFile;
        mTraceFile = nullptr;
        setRowCount(0);
    }
    else
        setRowCount(mTraceFile->Length());
    reloadData();
}

void TraceBrowser::gotoSlot()
{
    GotoDialog gotoDlg(this, false, true); // Problem: Cannot use when not debugging
    if(gotoDlg.exec() == QDialog::Accepted)
    {
        auto val = DbgValFromString(gotoDlg.expressionText.toUtf8().constData());
        if(val > 0 && val < mTraceFile->Length())
        {
            setSingleSelection(val);
            makeVisible(val);
        }
    }
}

void TraceBrowser::copyDisassemblySlot()
{
    QString clipboardHtml = QString("<div style=\"font-family: %1; font-size: %2px\">").arg(font().family()).arg(getRowHeight());
    QString clipboard = "";
    for(auto i = getSelectionStart(); i <= getSelectionEnd(); i++)
    {
        if(i != getSelectionStart())
        {
            clipboard += "\r\n";
            clipboardHtml += "<br/>";
        }
        RichTextPainter::List richText;
        unsigned char opcode[16];
        int opcodeSize;
        mTraceFile->OpCode(i, opcode, &opcodeSize);
        Instruction_t inst = mDisasm->DisassembleAt(opcode, opcodeSize, mTraceFile->Registers(i).regcontext.cip, 0);
        CapstoneTokenizer::TokenToRichText(inst.tokens, richText, 0);
        RichTextPainter::htmlRichText(richText, clipboardHtml, clipboard);
    }
    clipboardHtml += QString("</div>");
    Bridge::CopyToClipboard(clipboard, clipboardHtml);
}
