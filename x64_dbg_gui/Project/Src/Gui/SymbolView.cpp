#include "SymbolView.h"
#include "ui_SymbolView.h"

SymbolView::SymbolView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SymbolView)
{
    ui->setupUi(this);

    // Set main layout
    mMainLayout = new QVBoxLayout;
    mMainLayout->setContentsMargins(0, 0, 0, 0);
    mMainLayout->addWidget(ui->mainSplitter);
    setLayout(mMainLayout);

    // Create reference view
    mSearchListView = new SearchListView();
    mSearchListView->mSearchStartCol = 1;

    // Get font information
    QFont wFont("Monospace", 8);
    wFont.setStyleHint(QFont::Monospace);
    wFont.setFixedPitch(true);
    int charwidth=QFontMetrics(wFont).width(QChar(' '));

    // Create module list
    mModuleList = new StdTable();
    mModuleList->addColumnAt(charwidth*2*sizeof(int_t)+8, "Base", false);
    mModuleList->addColumnAt(0, "Module", true);

    // Setup symbol list
    mSearchListView->mList->addColumnAt(charwidth*2*sizeof(int_t)+8, "Address", true);
    mSearchListView->mList->addColumnAt(charwidth*80, "Symbol", true);
    mSearchListView->mList->addColumnAt(0, "Symbol (undecorated)", true);

    // Setup search list
    mSearchListView->mSearchList->addColumnAt(charwidth*2*sizeof(int_t)+8, "Address", true);
    mSearchListView->mSearchList->addColumnAt(charwidth*80, "Symbol", true);
    mSearchListView->mSearchList->addColumnAt(0, "Symbol (undecorated)", true);

    // Setup list splitter
    ui->listSplitter->addWidget(mModuleList);
    ui->listSplitter->addWidget(mSearchListView);
#ifdef _WIN64
    // mModuleList : mSymbolList = 40 : 100
    ui->listSplitter->setStretchFactor(0, 40);
    ui->listSplitter->setStretchFactor(1, 100);
#else
    // mModuleList : mSymbolList = 30 : 100
    ui->listSplitter->setStretchFactor(0, 30);
    ui->listSplitter->setStretchFactor(1, 100);
#endif //_WIN64

    // Setup log edit
    ui->symbolLogEdit->setFont(wFont);
    ui->symbolLogEdit->setStyleSheet("QTextEdit { background-color: rgb(255, 251, 240) }");
    ui->symbolLogEdit->setUndoRedoEnabled(false);
    ui->symbolLogEdit->setReadOnly(true);
    // Log : List = 2 : 9
    ui->mainSplitter->setStretchFactor(1, 9);
    ui->mainSplitter->setStretchFactor(0, 2);

    //setup context menu
    setupContextMenu();

    //Signals and slots
    connect(Bridge::getBridge(), SIGNAL(addMsgToSymbolLog(QString)), this, SLOT(addMsgToSymbolLogSlot(QString)));
    connect(Bridge::getBridge(), SIGNAL(clearLog()), this, SLOT(clearSymbolLogSlot()));
    connect(Bridge::getBridge(), SIGNAL(clearSymbolLog()), this, SLOT(clearSymbolLogSlot()));
    connect(mModuleList, SIGNAL(selectionChangedSignal(int)), this, SLOT(moduleSelectionChanged(int)));
    connect(Bridge::getBridge(), SIGNAL(updateSymbolList(int,SYMBOLMODULEINFO*)), this, SLOT(updateSymbolList(int,SYMBOLMODULEINFO*)));
    connect(Bridge::getBridge(), SIGNAL(setSymbolProgress(int)), ui->symbolProgress, SLOT(setValue(int)));
    connect(mSearchListView, SIGNAL(listContextMenuSignal(QPoint)), this, SLOT(symbolContextMenu(QPoint)));
    connect(mSearchListView, SIGNAL(enterPressedSignal()), this, SLOT(symbolFollow()));
}

SymbolView::~SymbolView()
{
    delete ui;
}

void SymbolView::setupContextMenu()
{
    mFollowSymbolAction = new QAction("&Follow in Disassembler", this);
    connect(mFollowSymbolAction, SIGNAL(triggered()), this, SLOT(symbolFollow()));

    mFollowSymbolDumpAction = new QAction("Follow in &Dump", this);
    connect(mFollowSymbolDumpAction, SIGNAL(triggered()), this, SLOT(symbolFollowDump()));

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
    mSearchListView->mList->setRowCount(0);
    DbgSymbolEnum(mModuleBaseList.at(index), cbSymbolEnum, mSearchListView->mList);
    mSearchListView->mList->reloadData();
    mSearchListView->mList->setSingleSelection(0);
    mSearchListView->mList->setTableOffset(0);
    mSearchListView->mList->setFocus();
    mSearchListView->mSearchBox->setText("");
}

void SymbolView::updateSymbolList(int module_count, SYMBOLMODULEINFO* modules)
{
    mModuleList->setRowCount(module_count);
    if(!module_count)
    {
        mSearchListView->mList->setRowCount(0);
        mSearchListView->mList->setSingleSelection(0);
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
    if(!mSearchListView->mCurList->getRowCount())
        return;
    QMenu* wMenu = new QMenu(this);
    wMenu->addAction(mFollowSymbolAction);
    wMenu->addAction(mFollowSymbolDumpAction);
    wMenu->addSeparator();
    wMenu->addAction(mCopySymbolAddress);
    wMenu->addAction(mCopyDecoratedSymbolAction);
    if(mSearchListView->mCurList->getCellContent(mSearchListView->mCurList->getInitialSelection(), 2).length())
        wMenu->addAction(mCopyUndecoratedSymbolAction);
    wMenu->exec(pos);
}

void SymbolView::symbolFollow()
{
    DbgCmdExecDirect(QString("disasm " + mSearchListView->mCurList->getCellContent(mSearchListView->mCurList->getInitialSelection(), 0)).toUtf8().constData());
    emit showCpu();
}

void SymbolView::symbolFollowDump()
{
    DbgCmdExecDirect(QString("dump " + mSearchListView->mCurList->getCellContent(mSearchListView->mCurList->getInitialSelection(), 0)).toUtf8().constData());
    emit showCpu();
}

void SymbolView::symbolAddressCopy()
{
    Bridge::CopyToClipboard(mSearchListView->mCurList->getCellContent(mSearchListView->mCurList->getInitialSelection(), 0).toUtf8().constData());
}

void SymbolView::symbolDecoratedCopy()
{
    Bridge::CopyToClipboard(mSearchListView->mCurList->getCellContent(mSearchListView->mCurList->getInitialSelection(), 1).toUtf8().constData());
}

void SymbolView::symbolUndecoratedCopy()
{
    Bridge::CopyToClipboard(mSearchListView->mCurList->getCellContent(mSearchListView->mCurList->getInitialSelection(), 2).toUtf8().constData());
}
