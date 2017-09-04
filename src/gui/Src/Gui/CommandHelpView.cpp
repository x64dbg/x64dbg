#include <QVBoxLayout>
#include "CommandHelpView.h"
#include "ui_CommandHelpView.h"
#include "SearchListView.h"

CommandHelpView::CommandHelpView(QWidget* parent) : QWidget(parent), ui(new Ui::CommandHelpView)
{
    ui->setupUi(this);

    mCurrentMode = 0;

    // Set main layout
    mMainLayout = new QVBoxLayout;
    mMainLayout->setContentsMargins(0, 0, 0, 0);
    mMainLayout->addWidget(ui->mainSplitter);
    setLayout(mMainLayout);

    // Create reference view
    mSearchListView = new SearchListView(true, this);
    mSearchListView->mSearchStartCol = 1;

    // Get font information
    QFont wFont("Monospace", 8, QFont::Normal, false);
    wFont.setStyleHint(QFont::Monospace);
    wFont.setFixedPitch(true);
    //int charwidth=QFontMetrics(wFont).width(QChar(' '));

    // Create module list
    mModuleList = new StdTable();
    mModuleList->addColumnAt(0, tr("Module"), true);

    // Setup symbol list
    mSearchListView->mList->addColumnAt(0, tr("Command"), true);

    // Setup search list
    mSearchListView->mSearchList->addColumnAt(0, tr("Command"), true);

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
    ui->mainSplitter->setStretchFactor(0, 1);

    connect(mModuleList, SIGNAL(selectionChangedSignal(int)), this, SLOT(moduleSelectionChanged(int)));
    connect(mSearchListView->mList, SIGNAL(selectionChangedSignal(int)), this, SLOT(symbolSelectionChanged(int)));

    //fill with example data
    mModuleList->setRowCount(2);
    mModuleList->setCellContent(0, 0, "x64dbg");
    mModuleList->setCellContent(1, 0, "testplugin");

    mModuleList->setSingleSelection(0);
}

CommandHelpView::~CommandHelpView()
{
    delete ui;
}

void CommandHelpView::moduleSelectionChanged(int index)
{
    mSearchListView->mList->setRowCount(0);

    if(index == 0) //x64dbg
    {
        mCurrentMode = 0;
        mSearchListView->mList->setRowCount(3);
        mSearchListView->mList->setCellContent(0, 0, "InitDebug");
        mSearchListView->mList->setCellContent(1, 0, "StopDebug");
        mSearchListView->mList->setCellContent(2, 0, "run");
    }
    else if(index == 1) //testplugin
    {
        mCurrentMode = 1;
        mSearchListView->mList->setRowCount(2);
        mSearchListView->mList->setCellContent(0, 0, "plugin1");
        mSearchListView->mList->setCellContent(1, 0, "grs");
    }

    mSearchListView->mList->reloadData();
    mSearchListView->mList->setSingleSelection(0);
    mSearchListView->mList->setTableOffset(0);
    mSearchListView->mList->setFocus();
    mSearchListView->mSearchBox->setText("");
}

void CommandHelpView::symbolSelectionChanged(int index)
{
    QString info = "";
    if(mCurrentMode == 0) //x64dbg
    {
        switch(index)
        {
        case 0: //InitDebug
            info = tr("Initialize debugging a file.\n\nExample:\nInitDebug \"C:\\test.exe\", commandline, \"C:\\homeDir\"");
            break;
        case 1: //StopDebug
            info = tr("Stop debugging (terminate the target).\n\nExample:\nStopDebug");
            break;
        case 2: //run
            info = tr("Resume debugging.\n\nExample:\nrun");
            break;
        }
    }
    else if(mCurrentMode == 1) //testplugin
    {
        switch(index)
        {
        case 0: //plugin1
            info = "Just a simple plugin test command.\n\nExample:\nplugin1";
            break;
        case 1: //grs
            info = "Get relocation table size.\n\nExample:\ngrs 404000";
            break;
        }

    }
    ui->symbolLogEdit->setText(info);
}
