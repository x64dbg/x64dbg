#include "MainWindow.h"
#include "./ui_MainWindow.h"
#include <QDebug>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QFileDialog>
#include "StringUtil.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setupNavigation();
    setupWidgets();
    setupToolSync();

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

void MainWindow::closeEvent(QCloseEvent* event)
{
    mToolSync->disconnect();
    QMainWindow::closeEvent(event);
}

void MainWindow::on_actionLoad_DMP_triggered()
{
    auto dumpFile = QFileDialog::getOpenFileName(this, "Load dump", QString(), "Dump Files (*.dmp)");
    if(!dumpFile.isEmpty())
    {
        loadFile(dumpFile);
    }
}

void MainWindow::loadFile(const QString & path)
{
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(this, "Error", "Failed to open dump file");
        return;
    }

    mDumpData.resize(file.size());
    file.read((char*)mDumpData.data(), mDumpData.size());
    std::string error;
    mParser = FileParser::Create(mDumpData.data(), mDumpData.data() + mDumpData.size(), error);
    if(!mParser)
    {
        QMessageBox::critical(this, "Error", QString::fromStdString(error));
        return;
    }

    // Reload the views
    auto parser = mParser.get();
    mMemoryMap->loadFileParser(parser);
    mHexDump->loadFileParser(parser);
    mDisassembly->loadFileParser(parser);
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
        case Navigation::Disassembly:
            ui->tabWidget->setCurrentWidget(mDisassembly);
            break;
        case Navigation::MemoryMap:
            ui->tabWidget->setCurrentWidget(mMemoryMap);
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
            mHexDump->printDumpAt(address);
            break;
        case Navigation::Disassembly:
            mDisassembly->disassembleAt(address, true, -1);
            break;
        case Navigation::MemoryMap:
            mMemoryMap->gotoAddress(address);
            break;
        default:
            qDebug() << "Unknown window: " << window;
            break;
        }
    });
}

void MainWindow::setupWidgets()
{
    mMemoryMap = new MiniMemoryMap(mNavigation, this);
    ui->tabWidget->addTab(mMemoryMap, "Memory Map");

    mHexDump = new MiniHexDump(mNavigation, GlobalArchitecture(), this);
    ui->tabWidget->addTab(mHexDump, "Dump");

    mDisassembly = new MiniDisassembly(mNavigation, GlobalArchitecture(), this);
    ui->tabWidget->addTab(mDisassembly, "Disassembly");
}

void MainWindow::setupToolSync()
{
    mToolSync = new REToolSync(this);
    connect(mToolSync, &REToolSync::info, [](QString message)
    {
        qDebug() << "REToolSync info: " + message;
    });
    connect(mToolSync, &REToolSync::error, [](QString message)
    {
        qDebug() << "REToolSync error: " + message;
    });
    connect(mToolSync, &REToolSync::gotoAddress, this, [this](duint address)
    {
        qDebug() << "REToolSync goto: " + ToPtrString(address);

        emit mNavigation->gotoAddress(Navigation::Dump, address);
        emit mNavigation->gotoAddress(Navigation::MemoryMap, address);
        emit mNavigation->gotoAddress(Navigation::Disassembly, address);
    });
#ifndef Q_OS_WASM
    if(!mToolSync->connect("http://localhost:6969"))
    {
        qDebug() << "Failed to connect to REToolSync";
    }
#endif
}
