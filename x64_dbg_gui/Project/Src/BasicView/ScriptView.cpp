#include "ScriptView.h"

ScriptView::ScriptView(StdTable *parent) : StdTable(parent)
{
    enableMultiSelection(false);

    int charwidth=QFontMetrics(this->font()).width(QChar(' '));

    addColumnAt(8+charwidth*4, "Line", false);
    addColumnAt(8+charwidth*60, "Text", false);
    addColumnAt(8+charwidth*40, "Info", false);

    setIp(0); //no IP

    connect(Bridge::getBridge(), SIGNAL(scriptAddLine(QString)), this, SLOT(addLine(QString)));
    connect(Bridge::getBridge(), SIGNAL(scriptClear()), this, SLOT(clear()));
    connect(Bridge::getBridge(), SIGNAL(scriptSetIp(int)), this, SLOT(setIp(int)));
    connect(Bridge::getBridge(), SIGNAL(scriptError(int,QString)), this, SLOT(error(int,QString)));
    connect(Bridge::getBridge(), SIGNAL(scriptSetTitle(QString)), this, SLOT(setTitle(QString)));
    connect(Bridge::getBridge(), SIGNAL(scriptSetInfoLine(int,QString)), this, SLOT(setInfoLine(int,QString)));

    setupContextMenu();
}

QString ScriptView::paintContent(QPainter* painter, int_t rowBase, int rowOffset, int col, int x, int y, int w, int h)
{
    bool wIsSelected=isSelected(rowBase, rowOffset);
    // Highlight if selected
    if(wIsSelected)
        painter->fillRect(QRect(x, y, w, h), QBrush(QColor("#C0C0C0")));
    QString returnString;
    switch(col)
    {
    case 0: //line number
    {
        int line=rowBase+rowOffset+1;
        returnString=returnString.sprintf("%.4d", line);
        painter->save();
        if(line==mIpLine) //IP
        {
            painter->fillRect(QRect(x, y, w, h), QBrush(QColor("#000000")));
            if(DbgScriptBpGet(line)) //breakpoint
                painter->setPen(QPen(QColor("#FF0000"))); //red address
            else
                painter->setPen(QPen(QColor("#FFFFFF"))); //black address
        }
        else if(DbgScriptBpGet(line)) //breakpoint
        {
            painter->fillRect(QRect(x, y, w, h), QBrush(QColor("#ff0000")));
            painter->setPen(QPen(QColor("#000000"))); //black address
        }
        else
        {
            if(wIsSelected)
                painter->setPen(QPen(QColor("#000000"))); //black address
            else
                painter->setPen(QPen(QColor("#808080")));
        }
        painter->drawText(QRect(x + 4, y , w - 4 , h), Qt::AlignVCenter | Qt::AlignLeft, returnString);
        painter->restore();
        returnString="";
    }
    break;

    case 1: //command
    {
        returnString=getCellContent(rowBase+rowOffset, col);
    }
    break;

    case 2: //info
    {
        returnString=getCellContent(rowBase+rowOffset, col);
    }
    break;
    }
    return returnString;
}

void ScriptView::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu* wMenu = new QMenu(this);
    wMenu->addMenu(mLoadMenu);
    if(getRowCount())
    {
        wMenu->addAction(mScriptUnload);
        wMenu->addSeparator();
        wMenu->addAction(mScriptBpToggle);
        wMenu->addAction(mScriptRunCursor);
        wMenu->addAction(mScriptStep);
        wMenu->addAction(mScriptRun);
        wMenu->addAction(mScriptAbort);
    }
    wMenu->addSeparator();
    wMenu->addAction(mScriptCmdExec);
    wMenu->exec(event->globalPos());
}

void ScriptView::setupContextMenu()
{
    mLoadMenu = new QMenu("Load Script", this);

    mScriptLoad = new QAction("Open...", this);
    mScriptLoad->setShortcutContext(Qt::WidgetShortcut);
    mScriptLoad->setShortcut(QKeySequence("ctrl+o"));
    this->addAction(mScriptLoad);
    connect(mScriptLoad, SIGNAL(triggered()), this, SLOT(openFile()));
    mLoadMenu->addAction(mScriptLoad);

    mScriptUnload = new QAction("Unload Script", this);
    mScriptUnload->setShortcutContext(Qt::WidgetShortcut);
    mScriptUnload->setShortcut(QKeySequence("ctrl+u"));
    this->addAction(mScriptUnload);
    connect(mScriptUnload, SIGNAL(triggered()), this, SLOT(unload()));

    mScriptRun = new QAction("Run", this);
    mScriptRun->setShortcutContext(Qt::WidgetShortcut);
    mScriptRun->setShortcut(QKeySequence("space"));
    this->addAction(mScriptRun);
    connect(mScriptRun, SIGNAL(triggered()), this, SLOT(run()));

    mScriptBpToggle = new QAction("Toggle BP", this);
    mScriptBpToggle->setShortcutContext(Qt::WidgetShortcut);
    mScriptBpToggle->setShortcut(QKeySequence("F2"));
    this->addAction(mScriptBpToggle);
    connect(mScriptBpToggle, SIGNAL(triggered()), this, SLOT(bpToggle()));

    mScriptRunCursor = new QAction("Run until selection", this);
    mScriptRunCursor->setShortcutContext(Qt::WidgetShortcut);
    mScriptRunCursor->setShortcut(QKeySequence("F4"));
    this->addAction(mScriptRunCursor);
    connect(mScriptRunCursor, SIGNAL(triggered()), this, SLOT(runCursor()));

    mScriptStep = new QAction("Step", this);
    mScriptStep->setShortcutContext(Qt::WidgetShortcut);
    mScriptStep->setShortcut(QKeySequence("tab"));
    this->addAction(mScriptStep);
    connect(mScriptStep, SIGNAL(triggered()), this, SLOT(step()));

    mScriptAbort = new QAction("Abort", this);
    mScriptAbort->setShortcutContext(Qt::WidgetShortcut);
    mScriptAbort->setShortcut(QKeySequence("esc"));
    this->addAction(mScriptAbort);
    connect(mScriptAbort, SIGNAL(triggered()), this, SLOT(abort()));

    mScriptCmdExec = new QAction("Execute Command...", this);
    mScriptCmdExec->setShortcutContext(Qt::WidgetShortcut);
    mScriptCmdExec->setShortcut(QKeySequence("x"));
    this->addAction(mScriptCmdExec);
    connect(mScriptCmdExec, SIGNAL(triggered()), this, SLOT(cmdExec()));
}

//slots
void ScriptView::addLine(QString text)
{
    int rows=getRowCount();
    setRowCount(rows+1);
    setCellContent(rows, 1, text);
    reloadData(); //repaint
}

void ScriptView::clear()
{
    setRowCount(0);
    mIpLine=0;
    reloadData(); //repaint
}

void ScriptView::setIp(int line)
{
    if(!isValidIndex(line-1, 0))
        mIpLine=0;
    else
        mIpLine=line;
    reloadData(); //repaint
}

void ScriptView::error(int line, QString message)
{
    QString title;
    if(isValidIndex(line-1, 0))
        title=title.sprintf("Error on line %.4d!", line);
    else
        title="Script Error!";
    QMessageBox msg(QMessageBox::Critical, title, message);
    msg.setWindowIcon(QIcon(":/icons/images/script-error.png"));
    msg.exec();
}

void ScriptView::setTitle(QString title)
{
    setWindowTitle(title);
}

void ScriptView::setInfoLine(int line, QString info)
{
    setCellContent(line-1, 2, info);
    reloadData(); //repaint
}

void ScriptView::openFile()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Select script"), 0, tr("Script files (*.txt *.scr);;All files (*.*)"));
    if(!filename.length())
        return;
    filename=QDir::toNativeSeparators(filename); //convert to native path format (with backlashes)
    DbgScriptUnload();
    if(!DbgScriptLoad(filename.toUtf8().constData()))
        error(0, "Failed to open script!");
}

void ScriptView::unload()
{
    DbgScriptUnload();
}

void ScriptView::run()
{
    DbgScriptRun(0);
}

void ScriptView::bpToggle()
{
    int selected=getInitialSelection()+1;
    if(!DbgScriptBpToggle(selected))
        error(selected, "Error setting script breakpoint!");
}

void ScriptView::runCursor()
{
    int selected=getInitialSelection()+1;
    DbgScriptRun(selected);
}

void ScriptView::step()
{
    DbgScriptStep();
}

void ScriptView::abort()
{
    DbgScriptAbort();
}

void ScriptView::cmdExec()
{
    LineEditDialog mLineEdit(this);
    mLineEdit.setWindowTitle("Execute Script Command...");
    if(mLineEdit.exec()!=QDialog::Accepted)
        return;
    if(!DbgScriptCmdExec(mLineEdit.editText.toUtf8().constData()))
        error(0, "Error executing command!");
}
