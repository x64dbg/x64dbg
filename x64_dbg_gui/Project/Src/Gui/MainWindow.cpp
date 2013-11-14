#include "MainWindow.h"
#include "ui_MainWindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->showMaximized();

    //Set window title
#ifdef _WIN64
    setWindowTitle("x64_dbg");
#else
    setWindowTitle("x32_dbg");
#endif

    //Load application icon
    HICON hIcon=LoadIcon(GetModuleHandleA(0), MAKEINTRESOURCE(100));
    SendMessageA((HWND)MainWindow::winId(), WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    DestroyIcon(hIcon);

    //Accept drops
    setAcceptDrops(true);

    // Memory Map View
    mMemMapView = new QMdiSubWindow();
    mMemMapView->setWindowTitle("Memory Map");
    mMemMapView->setWidget(new MemoryMapView());
    mMemMapView->setWindowIcon(QIcon(":/icons/images/memory-map.png"));
    mMemMapView->hide();
    mMemMapView->setGeometry(10, 10, 625, 500);

    // Log View
    mLogView = new QMdiSubWindow();
    mLogView->setWindowTitle("Log");
    mLogView->setWidget(new LogView());
    mLogView->setWindowIcon(QIcon(":/icons/images/alphabet/L.png"));
    mLogView->hide();
    mLogView->setGeometry(20, 20, 800, 300);

    mdiArea = new QMdiArea;
    mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    //Create QMdiSubWindow
    QMdiSubWindow* subWindow = new QMdiSubWindow();
    subWindow->setWindowTitle("CPU");
    subWindow->showMaximized();

    mCpuWin = new CPUWidget();
    mCpuWin->setWindowIcon(QIcon(":/icons/images/processor-cpu.png"));

    subWindow->setWidget(mCpuWin);

    //Add subWindow to Main QMdiArea here
    mdiArea->addSubWindow(subWindow);
    mdiArea->addSubWindow(mMemMapView);
    mdiArea->addSubWindow(mLogView);

    setCentralWidget(mdiArea);

    // Setup the command bar
    mCmdLineEdit = new CommandLineEdit(ui->cmdBar);
    ui->cmdBar->addWidget(new QLabel("Command: "));
    ui->cmdBar->addWidget(mCmdLineEdit);

    // Status bar
    mStatusLabel=new StatusLabel(ui->statusBar);
    mStatusLabel->setText("<font color='#ff0000'>Terminated</font>");
    ui->statusBar->addWidget(mStatusLabel);
    mLastLogLabel=new StatusLabel();
    ui->statusBar->addPermanentWidget(mLastLogLabel, 1);

    // Setup Signals/Slots
    connect(ui->actionStepOver, SIGNAL(triggered()), mCpuWin, SLOT(stepOverSlot()));
    connect(mCmdLineEdit, SIGNAL(returnPressed()), this, SLOT(executeCommand()));
    connect(ui->actionStepOver,SIGNAL(triggered()),this,SLOT(execStepOver()));
    connect(ui->actionStepInto,SIGNAL(triggered()),this,SLOT(execStepInto()));
    connect(ui->actionCommand,SIGNAL(triggered()),this,SLOT(setFocusToCommandBar()));
    connect(ui->actionClose,SIGNAL(triggered()),this,SLOT(execClose()));
    connect(ui->actionMemoryMap,SIGNAL(triggered()),this,SLOT(displayMemMapWidget()));
    connect(ui->actionRun,SIGNAL(triggered()),this,SLOT(execRun()));
    connect(ui->actionRtr,SIGNAL(triggered()),this,SLOT(execRtr()));
    connect(ui->actionLog,SIGNAL(triggered()),this,SLOT(displayLogWidget()));
    connect(ui->actionAbout,SIGNAL(triggered()),this,SLOT(displayAboutWidget()));
    connect(ui->actionOpen,SIGNAL(triggered()),this,SLOT(openFile()));
    connect(ui->actionPause,SIGNAL(triggered()),this,SLOT(execPause()));
    connect(ui->actionScylla,SIGNAL(triggered()),this,SLOT(startScylla()));
}


MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::executeCommand()
{
    QString wCmd = mCmdLineEdit->text();

    Bridge::getBridge()->execCmd(wCmd.toUtf8().constData());

    mCmdLineEdit->addCmdToHistory(wCmd);
    mCmdLineEdit->setText("");
}


void MainWindow::execStepOver()
{
    Bridge::getBridge()->execCmd("StepOver");
}

void MainWindow::execStepInto()
{
    Bridge::getBridge()->execCmd("StepInto");
}

void MainWindow::setFocusToCommandBar()
{
    mCmdLineEdit->setFocusToCmd();
}

void MainWindow::execClose()
{
    Bridge::getBridge()->execCmd("stop");
}

void MainWindow::execRun()
{
    Bridge::getBridge()->execCmd("run");
}

void MainWindow::execRtr()
{
    Bridge::getBridge()->execCmd("rtr");
}

void MainWindow::displayMemMapWidget()
{
    mMemMapView->widget()->show();
    mMemMapView->setFocus();
}

void MainWindow::displayLogWidget()
{
    mLogView->widget()->show();
    mLogView->setFocus();
}

void MainWindow::displayAboutWidget()
{
#ifdef _WIN64
    const char* title="About x64_dbg";
#else
    const char* title="About x32_dbg";
#endif
    MessageBoxA((HWND)MainWindow::winId(), "Created by:\nSigma (GUI)\nMr. eXoDia (DBG)\n\nSpecial Thanks:\nVisualPharm (http://visualpharm.com)\nReversingLabs (http://reversinglabs.com)\nBeatriX (http://beaengine.org)\nQt Project (http://qt-project.org)\nFugue Icons (http://yusukekamiyamane.com)\nNanomite (https://github.com/zer0fl4g/Nanomite)", title, MB_ICONINFORMATION);
}

void MainWindow::on_actionGoto_triggered()
{
    GotoDialog mGoto;
    if(mGoto.exec()==QDialog::Accepted)
    {
        QString cmd;
        Bridge::getBridge()->execCmd(cmd.sprintf("disasm \"%s\"", mGoto.expressionText.toUtf8().constData()).toUtf8().constData());
    }
}

void MainWindow::openFile()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Open file"), 0, tr("Executables (*.exe *.dll)"));
    if(!filename.length())
        return;
    filename=QDir::toNativeSeparators(filename); //convert to native path format (with backlashes)
    if(DbgIsDebugging())
    {
        Bridge::getBridge()->execCmd("stop"); //close current file (when present)
        Sleep(400);
    }
    QString cmd;
    Bridge::getBridge()->execCmd(cmd.sprintf("init \"%s\"", filename.toUtf8().constData()).toUtf8().constData());
}

void MainWindow::execPause()
{
    Bridge::getBridge()->execCmd("pause");
}

void MainWindow::startScylla() //this is executed
{
    Bridge::getBridge()->execCmd("StartScylla");
}

void MainWindow::dragEnterEvent(QDragEnterEvent* pEvent)
{
    if(pEvent->mimeData()->hasUrls())
    {
        pEvent->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent* pEvent)
{
    if(pEvent->mimeData()->hasUrls())
    {
        QString filename = QString(pEvent->mimeData()->data("FileName"));
        if(filename.contains(".exe", Qt::CaseInsensitive) or filename.contains(".dll", Qt::CaseInsensitive))
        {
            if(DbgIsDebugging())
            {
                Bridge::getBridge()->execCmd("stop"); //close current file (when present)
                Sleep(400);
            }
            QString cmd;
            Bridge::getBridge()->execCmd(cmd.sprintf("init \"%s\"", filename.toUtf8().constData()).toUtf8().constData());
        }
        pEvent->acceptProposedAction();
    }
}
