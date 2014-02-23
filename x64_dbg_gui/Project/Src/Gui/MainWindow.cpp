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

    //Load recent files
    loadMRUList(16);

    //Accept drops
    setAcceptDrops(true);

    // Log View
    mLogView = new LogView();
    mLogView->setWindowTitle("Log");
    mLogView->setWindowIcon(QIcon(":/icons/images/log.png"));
    mLogView->hide();

    // Symbol view
    mSymbolView = new SymbolView();
    mSymbolView->setWindowTitle("Symbols");
    mSymbolView->setWindowIcon(QIcon(":/icons/images/pdb.png"));
    mSymbolView->hide();

    // Breakpoints
    mBreakpointsView = new BreakpointsView();
    mBreakpointsView->setWindowTitle("Breakpoints");
    mBreakpointsView->setWindowIcon(QIcon(":/icons/images/breakpoint.png"));
    mBreakpointsView->hide();

    // Memory Map View
    mMemMapView = new MemoryMapView();
    mMemMapView->setWindowTitle("Memory Map");
    mMemMapView->setWindowIcon(QIcon(":/icons/images/memory-map.png"));
    mMemMapView->hide();

    // Script view
    mScriptView = new ScriptView();
    mScriptView->setWindowTitle("Script");
    mScriptView->setWindowIcon(QIcon(":/icons/images/script-code.png"));
    mScriptView->hide();

    // CPU View
    mCpuWidget = new CPUWidget();
    mCpuWidget->setWindowTitle("CPU");
    mCpuWidget->setWindowIcon(QIcon(":/icons/images/processor-cpu.png"));

    mReferenceView = new ReferenceView();
    mReferenceView->setWindowTitle("References");
    mReferenceView->setWindowIcon(QIcon(":/icons/images/search.png"));

    //Create the tab widget
    mTabWidget = new QTabWidget();

    //Setup tabs
    mTabWidget->addTab(mCpuWidget, mCpuWidget->windowIcon(), mCpuWidget->windowTitle());
    mTabWidget->addTab(mLogView, mLogView->windowIcon(), mLogView->windowTitle());
    mTabWidget->addTab(mBreakpointsView, mBreakpointsView->windowIcon(), mBreakpointsView->windowTitle());
    mTabWidget->addTab(mMemMapView, mMemMapView->windowIcon(), mMemMapView->windowTitle());
    mTabWidget->addTab(mScriptView, mScriptView->windowIcon(), mScriptView->windowTitle());
    mTabWidget->addTab(mSymbolView, mSymbolView->windowIcon(), mSymbolView->windowTitle());
    mTabWidget->addTab(mReferenceView, mReferenceView->windowIcon(),mReferenceView->windowTitle());

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
    connect(ui->actionSymbolInfo,SIGNAL(triggered()),this,SLOT(displaySymbolWidget()));
    connect(mSymbolView,SIGNAL(showCpu()),this,SLOT(displayCpuWidget()));

    connect(Bridge::getBridge(), SIGNAL(updateWindowTitle(QString)), this, SLOT(updateWindowTitleSlot(QString)));
    connect(Bridge::getBridge(), SIGNAL(updateCPUTitle(QString)), this, SLOT(updateCPUTitleSlot(QString)));

    const char* errormsg=DbgInit();
    if(errormsg)
    {
        QMessageBox msg(QMessageBox::Critical, "DbgInit Error!", QString(errormsg));
        msg.setWindowIcon(QIcon(":/icons/images/compile-error.png"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags()&(~Qt::WindowContextHelpButtonHint));
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

//Reads recent files list from settings
void MainWindow::loadMRUList(int maxItems)
{
    mMaxMRU = maxItems;
    for(unsigned int i=0; i<mMaxMRU; i++)
    {
        char currentFile[MAX_PATH]="";
        if(!BridgeSettingGet("Recent Files", QString().sprintf("%.2d", i+1).toUtf8().constData(), currentFile))
            break;
        mMRUList.push_back(currentFile);
    }
    updateMRUMenu();
}

//save recent files to settings
void MainWindow::saveMRUList()
{
    int mruSize=mMRUList.size();
    for(int i=0; i<mruSize; i++)
        BridgeSettingSet("Recent Files", QString().sprintf("%.2d", i+1).toUtf8().constData(), mMRUList.at(i).toUtf8().constData());
}

void MainWindow::addMRUEntry(QString entry)
{
    //remove duplicate entry if it exists
    removeMRUEntry(entry);
    mMRUList.insert(mMRUList.begin(), entry);
    if (mMRUList.size() > mMaxMRU)
        mMRUList.erase(mMRUList.begin() + mMaxMRU, mMRUList.end());
}

void MainWindow::removeMRUEntry(QString entry)
{
    std::vector<QString>::iterator it;

    for (it = mMRUList.begin(); it != mMRUList.end(); ++it)
    {
        if ((*it) == entry)
        {
            mMRUList.erase(it);
            break;
        }
    }
}

void MainWindow::updateMRUMenu()
{
    if (mMaxMRU < 1) return;

    QMenu* fileMenu = this->menuBar()->findChild<QMenu*>(QString::fromWCharArray(L"menuFile"));
    if (fileMenu == NULL)
    {
        QMessageBox msg(QMessageBox::Critical, "Error!", "Failed to find menu!");
        msg.setWindowIcon(QIcon(":/icons/images/compile-error.png"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags()&(~Qt::WindowContextHelpButtonHint));
        msg.exec();
        return;
    }
    fileMenu = fileMenu->findChild<QMenu*>(QString::fromWCharArray(L"menuRecent_Files"));
    if (fileMenu == NULL)
    {
        QMessageBox msg(QMessageBox::Critical, "Error!", "Failed to find submenu!");
        msg.setWindowIcon(QIcon(":/icons/images/compile-error.png"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags()&(~Qt::WindowContextHelpButtonHint));
        msg.exec();
        return;
    }

    QList<QAction*> list = fileMenu->actions();
    for (int i = 1; i < list.length(); ++i)
        fileMenu->removeAction(list.at(i));

    //add items to list
    if (mMRUList.size() > 0)
    {
        list = fileMenu->actions();
        for (unsigned int index = 0; index < mMRUList.size(); ++index)
        {
            fileMenu->addAction(new QAction(mMRUList.at(index), this));
            fileMenu->actions().last()->setObjectName(QString("MRU").append(QString::number(index)));
            connect(fileMenu->actions().last(), SIGNAL(triggered()), this, SLOT(openFile()));
        }
    }
}

QString MainWindow::getMRUEntry(size_t index)
{
    QString path;

    if (index < mMRUList.size())
        path = mMRUList.at(index);

    return path;
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
    QString title="About x64_dbg";
#else
    QString title="About x32_dbg";
#endif
    QMessageBox msg(QMessageBox::Information, title, "Created by:\nSigma (GUI)\nMr. eXoDia (DBG)\n\nSpecial Thanks:\nVisualPharm (http://visualpharm.com)\nReversingLabs (http://reversinglabs.com)\nBeatriX (http://beaengine.org)\nQt Project (http://qt-project.org)\nFugue Icons (http://yusukekamiyamane.com)\nNanomite (https://github.com/zer0fl4g/Nanomite)");
    msg.setWindowIcon(QIcon(":/icons/images/information.png"));
    msg.setParent(this, Qt::Dialog);
    msg.setWindowFlags(msg.windowFlags()&(~Qt::WindowContextHelpButtonHint));
    msg.exec();
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
    QString lastPath, filename;
    QAction* fileToOpen = qobject_cast<QAction*>(sender());

    //if sender is from recent list directly open file, otherwise show dialog
    if (fileToOpen == NULL || !fileToOpen->objectName().startsWith("MRU") || !(fileToOpen->text().length()))
    {
        lastPath = (mMRUList.size() > 0) ? mMRUList.at(0) : 0;
        filename = QFileDialog::getOpenFileName(this, tr("Open file"), lastPath, tr("Executables (*.exe *.dll);;All files (*.*)"));
        if(!filename.length())
            return;
        filename=QDir::toNativeSeparators(filename); //convert to native path format (with backlashes)
    }
    else
    {
        filename = fileToOpen->text();
    }

    if(DbgIsDebugging())
        DbgCmdExecDirect("stop");
    QString cmd;
    DbgCmdExec(cmd.sprintf("init \"%s\"", filename.toUtf8().constData()).toUtf8().constData());

    //file is from recent menu
    if (fileToOpen != NULL && fileToOpen->objectName().startsWith("MRU"))
    {
        fileToOpen->setObjectName(fileToOpen->objectName().split("U").at(1));
        int index = fileToOpen->objectName().toInt();

        QString exists = getMRUEntry(index);
        if (exists.length() == 0)
        {
            addMRUEntry(filename);
            updateMRUMenu();
            saveMRUList();
        }

        fileToOpen->setObjectName(fileToOpen->objectName().prepend("MRU"));
    }

    //file is from open button
    bool update = true;
    if (fileToOpen == NULL || fileToOpen->objectName().compare("actionOpen") == 0)
        for(unsigned int i=0; i<mMRUList.size(); i++)
            if(mMRUList.at(i) == filename)
            {
                update = false;
                break;
            }
    if (update)
    {
        addMRUEntry(filename);
        updateMRUMenu();
        saveMRUList();
    }
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
    if(!BridgeSettingGet("Recent Files", "01", filename)) //most recent file
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

void MainWindow::displaySymbolWidget()
{
    mSymbolView->show();
    mSymbolView->setFocus();
    setTab(mSymbolView);
}
