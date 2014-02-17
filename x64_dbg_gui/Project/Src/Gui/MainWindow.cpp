#include "MainWindow.h"
#include "ui_MainWindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->showMaximized();

#ifdef _WIN64
    mWindowMainTitle="x64_dbg";
#else
    mWindowMainTitle="x32_dbg";
#endif

    //Set window title
    setWindowTitle(QString(mWindowMainTitle));

    //Load application icon
    HICON hIcon=LoadIcon(GetModuleHandleA(0), MAKEINTRESOURCE(100));
    SendMessageA((HWND)MainWindow::winId(), WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    DestroyIcon(hIcon);

    //Accept drops
    setAcceptDrops(true);

    // Log View
    mLogView = new LogView();
    mLogView->setWindowTitle("Log");
    mLogView->setWindowIcon(QIcon(":/icons/images/log.png"));
    mLogView->hide();
    mLogView->setGeometry(10, 10, 800, 300);

    // Breakpoints
    mBreakpointsView = new BreakpointsView();
    mBreakpointsView->setWindowTitle("Breakpoints");
    mBreakpointsView->setWindowIcon(QIcon(":/icons/images/breakpoint.png"));
    mBreakpointsView->hide();
    mBreakpointsView->setGeometry(20, 20, 800, 300);

    // Memory Map View
    mMemMapView = new MemoryMapView();
    mMemMapView->setWindowTitle("Memory Map");
    mMemMapView->setWindowIcon(QIcon(":/icons/images/memory-map.png"));
    mMemMapView->hide();
    mMemMapView->setGeometry(30, 30, 625, 500);

    // Script view
    mScriptView = new ScriptView();
    mScriptView->setWindowTitle("Script");
    mScriptView->setWindowIcon(QIcon(":/icons/images/script-code.png"));
    mScriptView->hide();

    // CPU View
    mCpuWidget = new CPUWidget();
    mCpuWidget->setWindowTitle("CPU");
    mCpuWidget->setWindowIcon(QIcon(":/icons/images/processor-cpu.png"));

    //Create the tab widget
    mTabWidget = new QTabWidget();

    //Setup tabs
    mTabWidget->addTab(mCpuWidget, mCpuWidget->windowIcon(), mCpuWidget->windowTitle());
    mTabWidget->addTab(mLogView, mLogView->windowIcon(), mLogView->windowTitle());
    mTabWidget->addTab(mBreakpointsView, mBreakpointsView->windowIcon(), mBreakpointsView->windowTitle());
    mTabWidget->addTab(mMemMapView, mMemMapView->windowIcon(), mMemMapView->windowTitle());
    mTabWidget->addTab(mScriptView, mScriptView->windowIcon(), mScriptView->windowTitle());

    setCentralWidget(mTabWidget);

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
    connect(ui->actionRestart,SIGNAL(triggered()),this,SLOT(restartDebugging()));
    connect(ui->actionBreakpoints,SIGNAL(triggered()),this,SLOT(displayBreakpointWidget()));
    connect(ui->actioneStepOver,SIGNAL(triggered()),this,SLOT(execeStepOver()));
    connect(ui->actioneStepInto,SIGNAL(triggered()),this,SLOT(execeStepInto()));
    connect(ui->actioneRun,SIGNAL(triggered()),this,SLOT(execeRun()));
    connect(ui->actioneRtr,SIGNAL(triggered()),this,SLOT(execeRtr()));
    connect(ui->actionScript,SIGNAL(triggered()),this,SLOT(displayScriptWidget()));
    connect(ui->actionRunSelection,SIGNAL(triggered()),mCpuWidget,SLOT(runSelection()));
    connect(ui->actionCpu,SIGNAL(triggered()),this,SLOT(displayCpuWidget()));

    connect(Bridge::getBridge(), SIGNAL(updateWindowTitle(QString)), this, SLOT(updateWindowTitleSlot(QString)));
    connect(Bridge::getBridge(), SIGNAL(updateCPUTitle(QString)), this, SLOT(updateCPUTitleSlot(QString)));

    const char* errormsg=DbgInit();
    if(errormsg)
    {
        QMessageBox msg(QMessageBox::Critical, "DbgInit Error!", QString(errormsg));
        msg.setWindowIcon(QIcon(":/icons/images/compile-error.png"));
        msg.exec();
        ExitProcess(1);
    }
}


MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setTab(QWidget* widget)
{
    for(int i=0; i<mTabWidget->count(); i++)
        if(mTabWidget->widget(i)==widget)
        {
            mTabWidget->setCurrentIndex(i);
            break;
        }
}


void MainWindow::executeCommand()
{
    QString wCmd = mCmdLineEdit->text();

    DbgCmdExec(wCmd.toUtf8().constData());

    mCmdLineEdit->addCmdToHistory(wCmd);
    mCmdLineEdit->setText("");
}


void MainWindow::execStepOver()
{
    DbgCmdExec("StepOver");
}

void MainWindow::execStepInto()
{
    DbgCmdExec("StepInto");
}

void MainWindow::setFocusToCommandBar()
{
    mCmdLineEdit->setFocusToCmd();
}

void MainWindow::execClose()
{
    DbgCmdExec("stop");
}

void MainWindow::execRun()
{
    DbgCmdExec("run");
}

void MainWindow::execRtr()
{
    DbgCmdExec("rtr");
}

void MainWindow::displayMemMapWidget()
{
    mMemMapView->show();
    mMemMapView->setFocus();
    setTab(mMemMapView);
}

void MainWindow::displayLogWidget()
{
    mLogView->show();
    mLogView->setFocus();
    setTab(mLogView);
}

void MainWindow::displayScriptWidget()
{
    mScriptView->show();
    mScriptView->setFocus();
    setTab(mScriptView);
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
    GotoDialog mGoto(this);
    if(mGoto.exec()==QDialog::Accepted)
    {
        QString cmd;
        DbgCmdExec(cmd.sprintf("disasm \"%s\"", mGoto.expressionText.toUtf8().constData()).toUtf8().constData());
    }
}

void MainWindow::openFile()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Open file"), 0, tr("Executables (*.exe *.dll);;All files (*.*)"));
    if(!filename.length())
        return;
    filename=QDir::toNativeSeparators(filename); //convert to native path format (with backlashes)
    if(DbgIsDebugging())
        DbgCmdExecDirect("stop");
    QString cmd;
    DbgCmdExec(cmd.sprintf("init \"%s\"", filename.toUtf8().constData()).toUtf8().constData());
}

void MainWindow::execPause()
{
    DbgCmdExec("pause");
}

void MainWindow::startScylla() //this is executed
{
    DbgCmdExec("StartScylla");
}

void MainWindow::restartDebugging()
{
    char filename[MAX_SETTING_SIZE]="";
    if(!BridgeSettingGet("Recent Files", "path", filename))
        return;
    if(DbgIsDebugging())
    {
        DbgCmdExec("stop"); //close current file (when present)
        Sleep(400);
    }
    QString cmd;
    DbgCmdExec(cmd.sprintf("init \"%s\"", filename).toUtf8().constData());
}

void MainWindow::displayBreakpointWidget()
{
    mBreakpointsView->show();
    mBreakpointsView->setFocus();
    setTab(mBreakpointsView);
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
        QString filename = QDir::toNativeSeparators(pEvent->mimeData()->urls()[0].toLocalFile());
        if(filename.contains(".exe", Qt::CaseInsensitive) || filename.contains(".dll", Qt::CaseInsensitive))
        {
            if(DbgIsDebugging())
                DbgCmdExecDirect("stop");
            QString cmd;
            DbgCmdExec(cmd.sprintf("init \"%s\"", filename.toUtf8().constData()).toUtf8().constData());
        }
        pEvent->acceptProposedAction();
    }
}

void MainWindow::updateWindowTitleSlot(QString filename)
{
    if(filename.length())
        setWindowTitle(QString(mWindowMainTitle)+QString(" - ")+filename);
    else
        setWindowTitle(QString(mWindowMainTitle));
}

void MainWindow::updateCPUTitleSlot(QString modname)
{
    if(modname.length())
        mCpuWidget->setWindowTitle(QString("CPU - ")+modname);
    else
        mCpuWidget->setWindowTitle(QString("CPU"));
}

void MainWindow::execeStepOver()
{
    DbgCmdExec("eStepOver");
}

void MainWindow::execeStepInto()
{
    DbgCmdExec("eStepInto");
}
void MainWindow::execeRun()
{
    DbgCmdExec("erun");
}

void MainWindow::execeRtr()
{
    DbgCmdExec("ertr");
}

void MainWindow::displayCpuWidget()
{
    mCpuWidget->show();
    mCpuWidget->setFocus();
    setTab(mCpuWidget);
}
