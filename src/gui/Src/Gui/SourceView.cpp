#include "SourceView.h"
#include <QFileDialog>
#include <QDesktopServices>
#include <QProcess>
#include <QInputDialog>
#include <memory>
#include "FileLines.h"
#include "Bridge.h"
#include "CommonActions.h"

SourceView::SourceView(QString path, duint addr, QWidget* parent)
    : AbstractStdTable(parent),
      mSourcePath(path),
      mModBase(DbgFunctions()->ModBaseFromAddr(addr))
{
    enableMultiSelection(true);
    enableColumnSorting(false);
    setDrawDebugOnly(false);
    setAddressColumn(0);

    int charwidth = getCharWidth();

    addColumnAt(8 + charwidth * sizeof(duint) * 2, tr("Address"), false);
    addColumnAt(8 + charwidth * 8, tr("Line"), false);
    addColumnAt(0, tr("Code"), false);
    loadColumnFromConfig("SourceView");
    setupContextMenu();

    connect(this, SIGNAL(contextMenuSignal(QPoint)), this, SLOT(contextMenuSlot(QPoint)));
    connect(this, SIGNAL(doubleClickedSignal()), this, SLOT(followDisassemblerSlot()));
    connect(this, SIGNAL(enterPressedSignal()), this, SLOT(followDisassemblerSlot()));
    connect(Bridge::getBridge(), SIGNAL(updateDisassembly()), this, SLOT(reloadData()));

    Initialize();

    loadFile();
}

SourceView::~SourceView()
{
    clear();
}

QString SourceView::getCellContent(int r, int c)
{
    if(!isValidIndex(r, c))
        return QString();
    LineData & line = mLines.at(r - mPrepareTableOffset);
    switch(c)
    {
    case ColAddr:
        return line.addr ? ToPtrString(line.addr) : QString();
    case ColLine:
        return QString("%1").arg(line.index + 1);
    case ColCode:
        return line.code.code;
    }
    __debugbreak();
    return "INVALID";
}

bool SourceView::isValidIndex(int r, int c)
{
    if(!mFileLines)
        return false;
    if(c < ColAddr || c > ColCode)
        return false;
    return r >= 0 && size_t(r) < mFileLines->size();
}

void SourceView::sortRows(int column, bool ascending)
{
    Q_UNUSED(column);
    Q_UNUSED(ascending);
}

void SourceView::prepareData()
{
    AbstractTableView::prepareData();
    if(mFileLines)
    {
        auto lines = getNbrOfLineToPrint();
        mPrepareTableOffset = getTableOffset();
        mLines.clear();
        mLines.resize(lines);
        for(auto i = 0; i < lines; i++)
            parseLine(mPrepareTableOffset + i, mLines[i]);
    }
}

void SourceView::setSelection(duint addr)
{
    int line = 0;
    if(!DbgFunctions()->GetSourceFromAddr(addr, nullptr, &line))
        return;
    scrollSelect(line - 1);
    reloadData(); //repaint
}

void SourceView::clear()
{
    delete mFileLines;
    mFileLines = nullptr;
    mSourcePath.clear();
    mModBase = 0;
}

QString SourceView::getSourcePath()
{
    return mSourcePath;
}

void SourceView::contextMenuSlot(const QPoint & pos)
{
    QMenu menu(this);
    mMenuBuilder->build(&menu);
    menu.exec(mapToGlobal(pos));
}

void SourceView::gotoLineSlot()
{
    bool ok = false;
    int line = QInputDialog::getInt(this, tr("Go to line"), tr("Line (decimal):"), getInitialSelection() + 1, 1, getRowCount() - 1, 1, &ok, Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    if(ok)
    {
        scrollSelect(line - 1);
        reloadData(); //repaint
    }
}

void SourceView::openSourceFileSlot()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(mSourcePath));
}

void SourceView::showInDirectorySlot()
{
    QStringList args;
    args << "/select," << QDir::toNativeSeparators(mSourcePath);
    auto process = new QProcess(this);
    process->start("explorer.exe", args);
}

void SourceView::setupContextMenu()
{
    mMenuBuilder = new MenuBuilder(this);
    mCommonActions = new CommonActions(this, getActionHelperFuncs(), [this]()
    {
        return addrFromIndex(getInitialSelection());
    });
    mCommonActions->build(mMenuBuilder, CommonActions::ActionDisasm | CommonActions::ActionDump | CommonActions::ActionBreakpoint | CommonActions::ActionLabel | CommonActions::ActionComment
                          | CommonActions::ActionBookmark | CommonActions::ActionMemoryMap | CommonActions::ActionNewOrigin | CommonActions::ActionNewThread);
    mMenuBuilder->addSeparator();
    mMenuBuilder->addAction(makeShortcutAction(DIcon("geolocation-goto"), tr("Go to line"), SLOT(gotoLineSlot()), "ActionGotoExpression"));
    mMenuBuilder->addAction(makeAction(DIcon("source"), tr("Open source file"), SLOT(openSourceFileSlot())));
    mMenuBuilder->addAction(makeAction(DIcon("source_show_in_folder"), tr("Show source file in directory"), SLOT(showInDirectorySlot())));
    mMenuBuilder->addSeparator();
    MenuBuilder* copyMenu = new MenuBuilder(this);
    setupCopyColumnMenu(copyMenu);
    mMenuBuilder->addMenu(makeMenu(DIcon("copy"), tr("&Copy")), copyMenu);
    mMenuBuilder->loadFromConfig();
}

void SourceView::parseLine(size_t index, LineData & line)
{
    QString lineText = QString::fromStdString((*mFileLines)[index]);
    line.addr = addrFromIndex(index);
    line.index = index;

    line.code.code.clear();
    for(int i = 0; i < lineText.length(); i++)
    {
        QChar ch = lineText[i];
        if(ch == '\t')
        {
            int col = line.code.code.length();
            int spaces = mTabSize - col % mTabSize;
            line.code.code.append(QString(spaces, ' '));
        }
        else
        {
            line.code.code.append(ch);
        }
    }
    //TODO: add syntax highlighting?
}

duint SourceView::addrFromIndex(size_t index)
{
    return DbgFunctions()->GetAddrFromLineEx(mModBase, mSourcePath.toUtf8().constData(), int(index + 1));
}

void SourceView::loadFile()
{
    if(!mSourcePath.length())
        return;
    if(mFileLines)
    {
        delete mFileLines;
        mFileLines = nullptr;
    }
    mFileLines = new FileLines();
    mFileLines->open(mSourcePath.toStdWString().c_str());
    if(!mFileLines->isopen())
    {
        SimpleWarningBox(this, tr("Error"), tr("Failed to open file!"));
        delete mFileLines;
        mFileLines = nullptr;
        return;
    }
    if(!mFileLines->parse())
    {
        SimpleWarningBox(this, tr("Error"), tr("Failed to parse file!"));
        delete mFileLines;
        mFileLines = nullptr;
        return;
    }
    setRowCount(mFileLines->size());
    setTableOffset(0);
    reloadData();
}
