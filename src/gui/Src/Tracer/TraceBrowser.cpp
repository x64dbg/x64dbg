#include "TraceBrowser.h"
#include "TraceFileReader.h"
#include "RichTextPainter.h"
#include "BrowseDialog.h"
#include "QBeaEngine.h"

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
    mMenuBuilder->addAction(makeAction(DIcon("open.png"), tr("Open"), SLOT(openFileSlot())), [this](QMenu*)
    {
        return mTraceFile == nullptr;
    });
    mMenuBuilder->addAction(makeAction(DIcon("close.png"), tr("Close"), SLOT(closeFileSlot())), [this](QMenu*)
    {
        return mTraceFile != nullptr;
    });
}

void TraceBrowser::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu(this);
    mMenuBuilder->build(&menu);
    menu.exec(event->globalPos());
}

void TraceBrowser::mousePressEvent(QMouseEvent* event)
{
    dsint index = getIndexOffsetFromY(transY(event->y()));
    switch(event->button())
    {
    case Qt::LeftButton:
        if(index + getTableOffset() < getRowCount())
        {
            mSelection.firstSelectedIndex = index + getTableOffset();
            mSelection.fromIndex = index + getTableOffset();
            mSelection.toIndex = index + getTableOffset();
            updateViewport();
            break;
        }
    }
}

void TraceBrowser::mouseReleaseEvent(QMouseEvent* event)
{
    AbstractTableView::mouseReleaseEvent(event);
    updateViewport();
}

void TraceBrowser::updateColors()
{
    AbstractTableView::updateColors();
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
