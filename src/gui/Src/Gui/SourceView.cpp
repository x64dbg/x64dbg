#include "SourceView.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QProcess>
#include <QInputDialog>
#include <memory>
#include "FileLines.h"

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
    QMenu wMenu(this);
    mMenuBuilder->build(&wMenu);
    wMenu.exec(mapToGlobal(pos));
}

void SourceView::followDisassemblerSlot()
{
    duint addr = addrFromIndex(getInitialSelection());
    DbgCmdExec(QString("disasm %1").arg(ToPtrString(addr)).toUtf8().constData());
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
    mMenuBuilder->addAction(makeAction(DIcon(ArchValue("processor32.png", "processor64.png")), tr("&Follow in Disassembler"), SLOT(followDisassemblerSlot())), [this](QMenu*)
    {
        return DbgMemIsValidReadPtr(addrFromIndex(getInitialSelection()));
    });
    mMenuBuilder->addAction(makeShortcutAction(DIcon("geolocation-goto.png"), tr("Go to line"), SLOT(gotoLineSlot()), "ActionGotoExpression"));
    mMenuBuilder->addAction(makeAction(DIcon("source.png"), tr("Open source file"), SLOT(openSourceFileSlot())));
    mMenuBuilder->addAction(makeAction(DIcon("source_show_in_folder.png"), tr("Show source file in directory"), SLOT(showInDirectorySlot())));
    mMenuBuilder->loadFromConfig();
}

void SourceView::parseLine(size_t index, LineData & line)
{
    QString lineText = QString::fromStdString((*mFileLines)[index]);
    line.addr = addrFromIndex(index);
    line.index = index;
    line.code.code = lineText.replace('\t', "    "); //TODO: add syntax highlighting
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
        QMessageBox::warning(this, "Error", "Failed to open file!");
        delete mFileLines;
        mFileLines = nullptr;
        return;
    }
    if(!mFileLines->parse())
    {
        QMessageBox::warning(this, "Error", "Failed to parse file!");
        delete mFileLines;
        mFileLines = nullptr;
        return;
    }
    setRowCount(mFileLines->size());
    setTableOffset(0);
    reloadData();
}
