#include "MainWindow.h"
#include "./ui_MainWindow.h"
#include <QDebug>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QSplitter>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QCheckBox>
#include <QSpacerItem>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setupNavigation();
    setupWidgets();

    // Load the dump provided on the command line
    auto args = qApp->arguments();
    if(args.length() > 1)
    {
        loadFile(args.at(1));
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::loadFile(const QString & path)
{
    DbgSetMemoryProvider(nullptr);

    if(mFile != nullptr)
    {
        delete mFile;
        mFile = nullptr;
    }

    duint virtualBase = 0;
    try
    {
        mFile = new File(virtualBase, path);
    }
    catch(const std::exception & x)
    {
        QMessageBox::critical(this, tr("Error"), x.what());
    }

    DbgSetMemoryProvider(mFile);

    // Reload the views
    mHexDump->loadFile(mFile);
    emit mNavigation->gotoDump(virtualBase);
}

void MainWindow::setupNavigation()
{
    mNavigation = new Navigation(this);
    connect(mNavigation, &Navigation::focusWindow, [this](Navigation::Window window)
    {
        switch(window)
        {
        case Navigation::Dump:
            ui->tabWidget->setCurrentWidget(mHexDump);
            break;
        default:
            qDebug() << "Unknown window: " << window;
            break;
        }
    });
    connect(mNavigation, &Navigation::gotoAddress, [this](Navigation::Window window, duint address)
    {
        switch(window)
        {
        case Navigation::Dump:
            qDebug() << "Dump at: " << address;
            mHexDump->printDumpAt(address);
            break;
        default:
            qDebug() << "Unknown window: " << window;
            break;
        }
    });
}

struct DefaultArchitecture : Architecture
{
    bool disasm64() const override
    {
        return true;
    }

    bool addr64() const override
    {
        return true;
    }
} gArchitecture;

Architecture* GlobalArchitecture()
{
    return &gArchitecture;
}

void MainWindow::setupWidgets()
{
    mHexDump = new MiniHexDump(mNavigation, GlobalArchitecture(), this);
    mCodeEditor = new CodeEditor(this);
    mCodeEditor->setFont(Config()->monospaceFont());
    mHighlighter = new PatternHighlighter(mCodeEditor, mCodeEditor->document());
    mLogBrowser = new QTextBrowser(this);
    mLogBrowser->setFont(Config()->monospaceFont());
    mStructWidget = new StructWidget(this);

    mDataTable = new DataTable(this);
    connect(mHexDump, &MiniHexDump::selectionUpdated, [this]()
    {
        mDataTable->selectionChanged(mHexDump->getSelectionStart(), mHexDump->getSelectionEnd());
    });

    auto hl = new QHBoxLayout();
    //hl->addSpacing()
    hl->addStretch(1);
    hl->addWidget(new QCheckBox("Auto"));
    hl->addWidget(new QPushButton("Run"));
    hl->setContentsMargins(4, 4, 4, 0);

    auto vl = new QVBoxLayout();
    vl->addWidget(mCodeEditor);
    vl->addLayout(hl);
    vl->setContentsMargins(0, 0, 0, 0);
    vl->setSpacing(0);

    auto codeWidget = new QWidget();
    codeWidget->setLayout(vl);

    auto codeSplitter = new QSplitter(Qt::Vertical, this);
    codeSplitter->addWidget(codeWidget);
    codeSplitter->addWidget(mLogBrowser);
    codeSplitter->setStretchFactor(0, 80);
    codeSplitter->setStretchFactor(0, 20);

    auto structTabs = new QTabWidget(this);
    structTabs->addTab(mDataTable, "Data");
    structTabs->addTab(mStructWidget, "Struct");

    auto hexSplitter = new QSplitter(Qt::Vertical, this);
    hexSplitter->addWidget(mHexDump);
    hexSplitter->addWidget(structTabs);
    hexSplitter->setStretchFactor(0, 65);
    hexSplitter->setStretchFactor(1, 35);

    auto mainSplitter = new QSplitter(Qt::Horizontal, this);
    mainSplitter->addWidget(hexSplitter);
    mainSplitter->addWidget(codeSplitter);
    mainSplitter->setStretchFactor(0, 58);
    mainSplitter->setStretchFactor(1, 42);

    ui->tabWidget->addTab(mainSplitter, "Hex");
}

void MainWindow::on_action_Load_file_triggered()
{
    auto fileName = QFileDialog::getOpenFileName(this, "Load file", QString(), "All files (*)");
    if(!fileName.isEmpty())
    {
        loadFile(fileName);
    }
}


