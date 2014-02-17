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

    mModuleList = new StdTable();
    mModuleList->addColumnAt(0, "", false);
    mModuleList->setShowHeader(false); //hide header
    mModuleList->enableMultiSelection(false);

    int charwidth=QFontMetrics(wFont).width(QChar(' '));

    mSymbolList = new StdTable();
    mSymbolList->addColumnAt(charwidth*2*sizeof(int_t)+8, "Address", true);
    mSymbolList->addColumnAt(0, "Symbol", true);
    mSymbolList->enableMultiSelection(false);

    ui->listSplitter->addWidget(mModuleList);
    ui->listSplitter->addWidget(mSymbolList);
    // mModuleList : mSymbolList = 1 : 5
    ui->listSplitter->setStretchFactor(0, 1);
    ui->listSplitter->setStretchFactor(1, 5);

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
