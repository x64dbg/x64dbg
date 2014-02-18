#include "SymbolView.h"
#include "ui_SymbolView.h"

SymbolView::SymbolView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SymbolView)
{
    ui->setupUi(this);

    mainLayout = new QVBoxLayout;
    mainLayout->addWidget(ui->mainSplitter);
    setLayout(mainLayout);

    QFont wFont("Monospace", 8);
    wFont.setStyleHint(QFont::Monospace);
    wFont.setFixedPitch(true);

    int charwidth=QFontMetrics(wFont).width(QChar(' '));

    mModuleList = new StdTable();
    mModuleList->addColumnAt(charwidth*2*sizeof(int_t)+8, "Base", false);
    mModuleList->addColumnAt(0, "Module", true);

    mSymbolList = new StdTable();
    mSymbolList->addColumnAt(charwidth*2*sizeof(int_t)+8, "Address", true);
    mSymbolList->addColumnAt(charwidth*80, "Symbol", true);
    mSymbolList->addColumnAt(0, "Symbol (undecorated)", true);

    ui->listSplitter->addWidget(mModuleList);
    ui->listSplitter->addWidget(mSymbolList);
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

    connect(Bridge::getBridge(), SIGNAL(addMsgToSymbolLog(QString)), this, SLOT(addMsgToSymbolLogSlot(QString)));
    connect(Bridge::getBridge(), SIGNAL(clearLog()), this, SLOT(clearSymbolLogSlot()));
    connect(Bridge::getBridge(), SIGNAL(clearSymbolLog()), this, SLOT(clearSymbolLogSlot()));
    connect(mModuleList, SIGNAL(selectionChangedSignal(int)), this, SLOT(moduleSelectionChanged(int)));
    connect(Bridge::getBridge(), SIGNAL(updateSymbolList(int,SYMBOLMODULEINFO*)), this, SLOT(updateSymbolList(int,SYMBOLMODULEINFO*)));
    connect(Bridge::getBridge(), SIGNAL(setSymbolProgress(int)), ui->symbolProgress, SLOT(setValue(int)));
}

SymbolView::~SymbolView()
{
    delete ui;
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
    DbgSymbolEnum(moduleBaseList.at(index), cbSymbolEnum, mSymbolList);
    mSymbolList->reloadData();
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
    empty.swap(moduleBaseList);
    for(int i=0; i<module_count; i++)
    {
        moduleBaseList.push_back(modules[i].base);
        mModuleList->setCellContent(i, 0, QString("%1").arg(modules[i].base, sizeof(int_t)*2, 16, QChar('0')).toUpper());
        mModuleList->setCellContent(i, 1, modules[i].name);
    }
    mModuleList->reloadData();
    if(modules)
        BridgeFree(modules);
}
