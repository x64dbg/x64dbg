#include "SymbolView.h"
#include "ui_SymbolView.h"

SymbolView::SymbolView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SymbolView)
{
    ui->setupUi(this);

    mMainLayout = new QVBoxLayout;
    mMainLayout->setContentsMargins(0, 0, 0, 0);
    mMainLayout->addWidget(ui->mainSplitter);
    setLayout(mMainLayout);

    QFont wFont("Monospace", 8);
    wFont.setStyleHint(QFont::Monospace);
    wFont.setFixedPitch(true);

    int charwidth=QFontMetrics(wFont).width(QChar(' '));

    mModuleList = new StdTable();
    mModuleList->addColumnAt(charwidth*2*sizeof(int_t)+8, "Base", false);
    mModuleList->addColumnAt(0, "Module", true);

    // Create symbol list
    mSymbolList = new StdTable();
    mSymbolList->setContextMenuPolicy(Qt::CustomContextMenu);
    mSymbolList->addColumnAt(charwidth*2*sizeof(int_t)+8, "Address", true);
    mSymbolList->addColumnAt(charwidth*80, "Symbol", true);
    mSymbolList->addColumnAt(0, "Symbol (undecorated)", true);

    // Create search list
    mSymbolSearchList = new StdTable();
    mSymbolSearchList->addColumnAt(charwidth*2*sizeof(int_t)+8, "Address", true);
    mSymbolSearchList->addColumnAt(charwidth*80, "Symbol Search", true);
    mSymbolSearchList->addColumnAt(0, "Symbol (undecorated)", true);
    mSymbolSearchList->hide();

    // Create symbol layout
    mSymbolLayout = new QVBoxLayout();
    mSymbolLayout->setContentsMargins(0, 0, 0, 0);
    mSymbolLayout->setSpacing(0);
    mSymbolLayout->addWidget(mSymbolList);
    mSymbolLayout->addWidget(mSymbolSearchList);

    // Create symbol placeholder
    mSymbolPlaceHolder = new QWidget();
    mSymbolPlaceHolder->setLayout(mSymbolLayout);

    ui->listSplitter->addWidget(mModuleList);
    ui->listSplitter->addWidget(mSymbolPlaceHolder);
#ifdef _WIN64
    // mModuleList : mSymbolList = 40 : 100
    ui->listSplitter->setStretchFactor(0, 40);
    ui->listSplitter->setStretchFactor(1, 100);
#else
    // mModuleList : mSymbolList = 30 : 100
    ui->listSplitter->setStretchFactor(0, 30);
    ui->listSplitter->setStretchFactor(1, 100);
#endif //_WIN64

    ui->symbolLogEdit->setFont(wFont);
    ui->symbolLogEdit->setStyleSheet("QTextEdit { background-color: rgb(255, 251, 240) }");
    ui->symbolLogEdit->setUndoRedoEnabled(false);
    ui->symbolLogEdit->setReadOnly(true);

    // Log : List = 2 : 9
    ui->mainSplitter->setStretchFactor(0, 9);
    ui->mainSplitter->setStretchFactor(1, 2);

    //setup context menu
    setupContextMenu();

    //Signals and slots
    connect(Bridge::getBridge(), SIGNAL(addMsgToSymbolLog(QString)), this, SLOT(addMsgToSymbolLogSlot(QString)));
    connect(Bridge::getBridge(), SIGNAL(clearLog()), this, SLOT(clearSymbolLogSlot()));
    connect(Bridge::getBridge(), SIGNAL(clearSymbolLog()), this, SLOT(clearSymbolLogSlot()));
    connect(mModuleList, SIGNAL(selectionChangedSignal(int)), this, SLOT(moduleSelectionChanged(int)));
    connect(Bridge::getBridge(), SIGNAL(updateSymbolList(int,SYMBOLMODULEINFO*)), this, SLOT(updateSymbolList(int,SYMBOLMODULEINFO*)));
    connect(Bridge::getBridge(), SIGNAL(setSymbolProgress(int)), ui->symbolProgress, SLOT(setValue(int)));
    connect(mSymbolList, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(symbolContextMenu(const QPoint &)));
    connect(mSymbolList, SIGNAL(keyPressedSignal(QKeyEvent*)), this, SLOT(symbolKeyPressed(QKeyEvent*)));
    connect(mSymbolSearchList, SIGNAL(keyPressedSignal(QKeyEvent*)), this, SLOT(symbolKeyPressed(QKeyEvent*)));
    connect(ui->searchBox, SIGNAL(textChanged(QString)), this, SLOT(searchTextChanged(QString)));
}

SymbolView::~SymbolView()
{
    delete ui;
}

void SymbolView::setupContextMenu()
{
    mFollowSymbolAction = new QAction("&Follow in Disassembler", this);
    connect(mFollowSymbolAction, SIGNAL(triggered()), this, SLOT(symbolFollow()));

    mCopySymbolAddress = new QAction("Copy &Address", this);
    connect(mCopySymbolAddress, SIGNAL(triggered()), this, SLOT(symbolAddressCopy()));

    mCopyDecoratedSymbolAction = new QAction("Copy &Symbol", this);
    connect(mCopyDecoratedSymbolAction, SIGNAL(triggered()), this, SLOT(symbolDecoratedCopy()));

    mCopyUndecoratedSymbolAction = new QAction("Copy Symbol (&undecorated)", this);
    connect(mCopyUndecoratedSymbolAction, SIGNAL(triggered()), this, SLOT(symbolUndecoratedCopy()));
}

void SymbolView::addMsgToSymbolLogSlot(QString msg)
{
    ui->symbolLogEdit->moveCursor(QTextCursor::End);
    ui->symbolLogEdit->insertPlainText(msg);
}

void SymbolView::clearSymbolLogSlot()
{
    ui->symbolLogEdit->clear();
}

void SymbolView::cbSymbolEnum(SYMBOLINFO* symbol, void* user)
{
    StdTable* symbolList=(StdTable*)user;
    int_t index=symbolList->getRowCount();
    symbolList->setRowCount(index+1);
    symbolList->setCellContent(index, 0, QString("%1").arg(symbol->addr, sizeof(int_t)*2, 16, QChar('0')).toUpper());
    if(symbol->decoratedSymbol)
    {
        symbolList->setCellContent(index, 1, symbol->decoratedSymbol);
        BridgeFree(symbol->decoratedSymbol);
    }
    if(symbol->undecoratedSymbol)
    {
        symbolList->setCellContent(index, 2, symbol->undecoratedSymbol);
        BridgeFree(symbol->undecoratedSymbol);
    }
}

void SymbolView::moduleSelectionChanged(int index)
{
    mSymbolList->setRowCount(0);
    DbgSymbolEnum(mModuleBaseList.at(index), cbSymbolEnum, mSymbolList);
    mSymbolList->reloadData();
    mSymbolList->setSingleSelection(0);
    mSymbolList->setTableOffset(0);
    mSymbolList->setFocus();
    ui->searchBox->setText("");
}

void SymbolView::updateSymbolList(int module_count, SYMBOLMODULEINFO* modules)
{
    mModuleList->setRowCount(module_count);
    if(!module_count)
    {
        mSymbolList->setRowCount(0);
        mSymbolList->setSingleSelection(0);
        mModuleList->setSingleSelection(0);
    }
    QList<uint_t> empty;
    empty.clear();
    empty.swap(mModuleBaseList);
    for(int i=0; i<module_count; i++)
    {
        mModuleBaseList.push_back(modules[i].base);
        mModuleList->setCellContent(i, 0, QString("%1").arg(modules[i].base, sizeof(int_t)*2, 16, QChar('0')).toUpper());
        mModuleList->setCellContent(i, 1, modules[i].name);
    }
    mModuleList->reloadData();
    if(modules)
        BridgeFree(modules);
}

void SymbolView::symbolContextMenu(const QPoint & pos)
{
    if(!mSymbolList->getRowCount())
        return;
    QMenu* wMenu = new QMenu(this);
    wMenu->addAction(mFollowSymbolAction);
    wMenu->addSeparator();
    wMenu->addAction(mCopySymbolAddress);
    wMenu->addAction(mCopyDecoratedSymbolAction);
    if(mSymbolList->getCellContent(mSymbolList->getInitialSelection(), 2).length())
        wMenu->addAction(mCopyUndecoratedSymbolAction);
    wMenu->exec(mSymbolList->mapToGlobal(pos));
}

void SymbolView::symbolFollow()
{
    StdTable* curList;
    if(mSymbolList->isHidden()) //search is active
        curList=mSymbolSearchList;
    else
        curList=mSymbolList;
    DbgCmdExecDirect(QString("disasm " + curList->getCellContent(curList->getInitialSelection(), 0)).toUtf8().constData());
    emit showCpu();
}

void SymbolView::symbolAddressCopy()
{
    StdTable* curList;
    if(mSymbolList->isHidden()) //search is active
        curList=mSymbolSearchList;
    else
        curList=mSymbolList;
    Bridge::CopyToClipboard(curList->getCellContent(curList->getInitialSelection(), 0).toUtf8().constData());
}

void SymbolView::symbolDecoratedCopy()
{
    StdTable* curList;
    if(mSymbolList->isHidden()) //search is active
        curList=mSymbolSearchList;
    else
        curList=mSymbolList;
    Bridge::CopyToClipboard(curList->getCellContent(curList->getInitialSelection(), 1).toUtf8().constData());
}

void SymbolView::symbolUndecoratedCopy()
{
    StdTable* curList;
    if(mSymbolList->isHidden()) //search is active
        curList=mSymbolSearchList;
    else
        curList=mSymbolList;
    Bridge::CopyToClipboard(curList->getCellContent(curList->getInitialSelection(), 2).toUtf8().constData());
}

void SymbolView::symbolKeyPressed(QKeyEvent* event)
{
    char ch=event->text().toUtf8().constData()[0];
    if(isprint(ch)) //add a char to the search box
        ui->searchBox->setText(ui->searchBox->text()+QString(QChar(ch)));
    else if(event->key()==Qt::Key_Backspace) //remove a char from the search box
    {
        QString newText;
        if(event->modifiers()==Qt::ControlModifier) //clear the search box
            newText="";
        else
        {
            newText=ui->searchBox->text();
            newText.chop(1);
        }
        ui->searchBox->setText(newText);
    }
    else if((event->key()==Qt::Key_Return || event->key()==Qt::Key_Enter) && event->modifiers()==Qt::NoModifier) //user pressed enter
        mFollowSymbolAction->trigger();
}

void SymbolView::searchTextChanged(const QString &arg1)
{
    if(arg1.length())
    {
        mSymbolList->hide();
        mSymbolSearchList->show();
    }
    else
    {
        mSymbolSearchList->hide();
        mSymbolList->show();
        mSymbolList->setFocus();
    }
    mSymbolSearchList->setRowCount(0);
    int count=mSymbolList->getRowCount();
    for(int i=0,j=0; i<count; i++)
    {
        if(mSymbolList->getCellContent(i, 1).contains(arg1, Qt::CaseInsensitive) || mSymbolList->getCellContent(i, 2).contains(arg1, Qt::CaseInsensitive))
        {
            mSymbolSearchList->setRowCount(j+1);
            mSymbolSearchList->setCellContent(j, 0, mSymbolList->getCellContent(i, 0));
            mSymbolSearchList->setCellContent(j, 1, mSymbolList->getCellContent(i, 1));
            mSymbolSearchList->setCellContent(j, 2, mSymbolList->getCellContent(i, 2));
            j++;
        }
    }
    count=mSymbolSearchList->getRowCount();
    mSymbolSearchList->setTableOffset(0);
    for(int i=0; i<count; i++)
    {
        if(mSymbolSearchList->getCellContent(i, 1).startsWith(arg1, Qt::CaseInsensitive) || mSymbolSearchList->getCellContent(i, 2).startsWith(arg1, Qt::CaseInsensitive))
        {
            if(count>mSymbolSearchList->getViewableRowsCount())
            {
                int cur=i-mSymbolSearchList->getViewableRowsCount()/2;
                if(!mSymbolSearchList->isValidIndex(cur, 0))
                    cur=i;
                mSymbolSearchList->setTableOffset(cur);
            }
            mSymbolSearchList->setSingleSelection(i);
            break;
        }
    }
    mSymbolSearchList->reloadData();
    mSymbolSearchList->setFocus();
}
