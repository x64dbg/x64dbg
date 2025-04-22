#include "MainWindow.h"
#include "./ui_MainWindow.h"
#include <QDebug>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QFileDialog>

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

    if(mFile != nullptr) {
        delete mFile;
        mFile = nullptr;
    }

    duint virtualBase = 0;
    try
    {
        mFile = new File(virtualBase, path);
    }
    catch(const std::exception& x)
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

struct DefaultArchitecture : Architecture {
    bool disasm64() const override {
        return true;
    }

    bool addr64() const override {
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
    ui->tabWidget->addTab(mHexDump, "Dump");
}

void MainWindow::on_action_Load_file_triggered()
{
    auto fileName = QFileDialog::getOpenFileName(this, "Load file", QString(), "All files (*)");
    if(!fileName.isEmpty())
    {
        loadFile(fileName);
    }
}


