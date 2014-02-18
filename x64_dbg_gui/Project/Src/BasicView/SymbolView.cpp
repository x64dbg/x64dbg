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

    mModuleList->setRowCount(5);
    const char* modules[5]={"ntdll.dll", "kernel32.dll", "kernelbase.dll", "msvcrt.dll", "user32.dll"};
    const char* bases[5]={"77A70000", "773D0000", "762F0000", "75800000", "756F0000"};
    for(int i=0; i<5; i++)
    {
        mModuleList->setCellContent(i, 0, bases[i]);
        mModuleList->setCellContent(i, 1, modules[i]);
    }

    connect(Bridge::getBridge(), SIGNAL(addMsgToSymbolLog(QString)), this, SLOT(addMsgToSymbolLogSlot(QString)));
    connect(Bridge::getBridge(), SIGNAL(clearLog()), this, SLOT(clearSymbolLogSlot()));
    connect(Bridge::getBridge(), SIGNAL(clearSymbolLog()), this, SLOT(clearSymbolLogSlot()));
    connect(mModuleList, SIGNAL(selectionChangedSignal(int)), this, SLOT(moduleSelectionChanged(int)));

    emit mModuleList->selectionChangedSignal(0);
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

void SymbolView::moduleSelectionChanged(int index)
{
    const char* addr[5]={"77A8FAD8", "773E1826", "762FEF02", "75878D2C", "7575FD1E"};
    const char* data[5]={"ntdll.NtQueryInformationProcess", "kernel32.VirtualAlloc", "kernelbase.VirtualQueryEx", "msvcrt.puts", "user32.MessageBoxA"};
    mSymbolList->setRowCount(1);
    mSymbolList->setCellContent(0, 0, addr[index]);
    mSymbolList->setCellContent(0, 1, data[index]);
    mSymbolList->reloadData();
}

void SymbolView::updateSymbolList(int module_count, SYMBOLMODULEINFO* modules)
{
}
