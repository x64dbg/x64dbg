#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QMessageBox>
#include <QIcon>
#include <QUrl>
#include <QFileDialog>
#include <QMimeData>
#include <QDesktopServices>
#include "Configuration.h"
#include "SettingsDialog.h"
#include "AppearanceDialog.h"
#include "ShortcutsDialog.h"
#include "AttachDialog.h"
#include "LineEditDialog.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->showMaximized();

#ifdef _WIN64
    mWindowMainTitle = "x64_dbg";
#else
    mWindowMainTitle = "x32_dbg";
#endif

    //Set window title
    setWindowTitle(QString(mWindowMainTitle));

    //Load application icon
    HICON hIcon = LoadIcon(GetModuleHandleA(0), MAKEINTRESOURCE(100));
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
    connect(mBreakpointsView, SIGNAL(showCpu()), this, SLOT(displayCpuWidget()));

    // Memory Map View
    mMemMapView = new MemoryMapView();
    connect(mMemMapView, SIGNAL(showCpu()), this, SLOT(displayCpuWidget()));
    mMemMapView->setWindowTitle("Memory Map");
    mMemMapView->setWindowIcon(QIcon(":/icons/images/memory-map.png"));
    mMemMapView->hide();

    // Callstack View
    mCallStackView = new CallStackView();
    mCallStackView->setWindowTitle("Call Stack");
    mCallStackView->setWindowIcon(QIcon(":/icons/images/callstack.png"));
    connect(mCallStackView, SIGNAL(showCpu()), this, SLOT(displayCpuWidget()));

    // Script view
    mScriptView = new ScriptView();
    mScriptView->setWindowTitle("Script");
    mScriptView->setWindowIcon(QIcon(":/icons/images/script-code.png"));
    mScriptView->hide();

    // CPU View
    mCpuWidget = new CPUWidget();
    mCpuWidget->setWindowTitle("CPU");
    mCpuWidget->setWindowIcon(QIcon(":/icons/images/processor-cpu.png"));

    // Reference Manager
    mReferenceManager = new ReferenceManager();
    Bridge::getBridge()->referenceManager = mReferenceManager;
    mReferenceManager->setWindowTitle("References");
    mReferenceManager->setWindowIcon(QIcon(":/icons/images/search.png"));

    // Thread View
    mThreadView = new ThreadView();
    connect(mThreadView, SIGNAL(showCpu()), this, SLOT(displayCpuWidget()));
    mThreadView->setWindowTitle("Threads");
    mThreadView->setWindowIcon(QIcon(":/icons/images/arrow-threads.png"));

    //Create the tab widget
    mTabWidget = new MHTabWidget(NULL);

    //Setup tabs
    mTabWidget->addTab(mCpuWidget, mCpuWidget->windowIcon(), mCpuWidget->windowTitle());
    mTabWidget->addTab(mLogView, mLogView->windowIcon(), mLogView->windowTitle());
    mTabWidget->addTab(mBreakpointsView, mBreakpointsView->windowIcon(), mBreakpointsView->windowTitle());
    mTabWidget->addTab(mMemMapView, mMemMapView->windowIcon(), mMemMapView->windowTitle());
    mTabWidget->addTab(mCallStackView, mCallStackView->windowIcon(), mCallStackView->windowTitle());
    mTabWidget->addTab(mScriptView, mScriptView->windowIcon(), mScriptView->windowTitle());
    mTabWidget->addTab(mSymbolView, mSymbolView->windowIcon(), mSymbolView->windowTitle());
    mTabWidget->addTab(mReferenceManager, mReferenceManager->windowIcon(), mReferenceManager->windowTitle());
    mTabWidget->addTab(mThreadView, mThreadView->windowIcon(), mThreadView->windowTitle());

    setCentralWidget(mTabWidget);

    // Setup the command bar
    mCmdLineEdit = new CommandLineEdit(ui->cmdBar);
    ui->cmdBar->addWidget(new QLabel("Command: "));
    ui->cmdBar->addWidget(mCmdLineEdit);

    // Status bar
    mStatusLabel = new StatusLabel(ui->statusBar);
    mStatusLabel->setText("Ready");
    ui->statusBar->addWidget(mStatusLabel);
    mLastLogLabel = new StatusLabel();
    ui->statusBar->addPermanentWidget(mLastLogLabel, 1);

    mPatchDialog = new PatchDialog(this);
    mCalculatorDialog = new CalculatorDialog(this);
    connect(mCalculatorDialog, SIGNAL(showCpu()), this, SLOT(displayCpuWidget()));

    // Setup Signals/Slots
    connect(mCmdLineEdit, SIGNAL(returnPressed()), this, SLOT(executeCommand()));
    connect(ui->actionStepOver, SIGNAL(triggered()), this, SLOT(execStepOver()));
    connect(ui->actionStepInto, SIGNAL(triggered()), this, SLOT(execStepInto()));
    connect(ui->actionCommand, SIGNAL(triggered()), this, SLOT(setFocusToCommandBar()));
    connect(ui->actionClose, SIGNAL(triggered()), this, SLOT(execClose()));
    connect(ui->actionMemoryMap, SIGNAL(triggered()), this, SLOT(displayMemMapWidget()));
    connect(ui->actionRun, SIGNAL(triggered()), this, SLOT(execRun()));
    connect(ui->actionRtr, SIGNAL(triggered()), this, SLOT(execRtr()));
    connect(ui->actionLog, SIGNAL(triggered()), this, SLOT(displayLogWidget()));
    connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(displayAboutWidget()));
    connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(openFile()));
    connect(ui->actionPause, SIGNAL(triggered()), this, SLOT(execPause()));
    connect(ui->actionScylla, SIGNAL(triggered()), this, SLOT(startScylla()));
    connect(ui->actionRestart, SIGNAL(triggered()), this, SLOT(restartDebugging()));
    connect(ui->actionBreakpoints, SIGNAL(triggered()), this, SLOT(displayBreakpointWidget()));
    connect(ui->actioneStepOver, SIGNAL(triggered()), this, SLOT(execeStepOver()));
    connect(ui->actioneStepInto, SIGNAL(triggered()), this, SLOT(execeStepInto()));
    connect(ui->actioneRun, SIGNAL(triggered()), this, SLOT(execeRun()));
    connect(ui->actioneRtr, SIGNAL(triggered()), this, SLOT(execeRtr()));
    connect(ui->actionSkipNextInstruction, SIGNAL(triggered()), this, SLOT(execSkip()));
    connect(ui->actionScript, SIGNAL(triggered()), this, SLOT(displayScriptWidget()));
    connect(ui->actionRunSelection, SIGNAL(triggered()), this, SLOT(runSelection()));
    connect(ui->actionCpu, SIGNAL(triggered()), this, SLOT(displayCpuWidget()));
    connect(ui->actionSymbolInfo, SIGNAL(triggered()), this, SLOT(displaySymbolWidget()));
    connect(mSymbolView, SIGNAL(showCpu()), this, SLOT(displayCpuWidget()));
    connect(mReferenceManager, SIGNAL(showCpu()), this, SLOT(displayCpuWidget()));
    connect(ui->actionReferences, SIGNAL(triggered()), this, SLOT(displayReferencesWidget()));
    connect(ui->actionThreads, SIGNAL(triggered()), this, SLOT(displayThreadsWidget()));
    connect(ui->actionSettings, SIGNAL(triggered()), this, SLOT(openSettings()));
    connect(ui->actionStrings, SIGNAL(triggered()), this, SLOT(findStrings()));
    connect(ui->actionCalls, SIGNAL(triggered()), this, SLOT(findModularCalls()));
    connect(ui->actionAppearance, SIGNAL(triggered()), this, SLOT(openAppearance()));
    connect(ui->actionShortcuts, SIGNAL(triggered()), this, SLOT(openShortcuts()));
    connect(ui->actionCalculator, SIGNAL(triggered()), this, SLOT(openCalculator()));
    connect(ui->actionPatches, SIGNAL(triggered()), this, SLOT(patchWindow()));
    connect(ui->actionComments, SIGNAL(triggered()), this, SLOT(displayComments()));
    connect(ui->actionLabels, SIGNAL(triggered()), this, SLOT(displayLabels()));
    connect(ui->actionBookmarks, SIGNAL(triggered()), this, SLOT(displayBookmarks()));
    connect(ui->actionFunctions, SIGNAL(triggered()), this, SLOT(displayFunctions()));
    connect(ui->actionCheckUpdates, SIGNAL(triggered()), this, SLOT(checkUpdates()));
    connect(ui->actionCallStack, SIGNAL(triggered()), this, SLOT(displayCallstack()));
    connect(ui->actionDonate, SIGNAL(triggered()), this, SLOT(donate()));
    connect(ui->actionAttach, SIGNAL(triggered()), this, SLOT(displayAttach()));
    connect(ui->actionDetach, SIGNAL(triggered()), this, SLOT(detach()));
    connect(ui->actionChangeCommandLine, SIGNAL(triggered()), this, SLOT(changeCommandLine()));

    connect(Bridge::getBridge(), SIGNAL(updateWindowTitle(QString)), this, SLOT(updateWindowTitleSlot(QString)));
    connect(Bridge::getBridge(), SIGNAL(addRecentFile(QString)), this, SLOT(addRecentFile(QString)));
    connect(Bridge::getBridge(), SIGNAL(setLastException(uint)), this, SLOT(setLastException(uint)));
    connect(Bridge::getBridge(), SIGNAL(menuAddMenu(int, QString)), this, SLOT(addMenu(int, QString)));
    connect(Bridge::getBridge(), SIGNAL(menuAddMenuEntry(int, QString)), this, SLOT(addMenuEntry(int, QString)));
    connect(Bridge::getBridge(), SIGNAL(menuAddSeparator(int)), this, SLOT(addSeparator(int)));
    connect(Bridge::getBridge(), SIGNAL(menuClearMenu(int)), this, SLOT(clearMenu(int)));
    connect(mCpuWidget->mDisas, SIGNAL(displayReferencesWidget()), this, SLOT(displayReferencesWidget()));
    connect(mCpuWidget->mDisas, SIGNAL(showPatches()), this, SLOT(patchWindow()));
    connect(mCpuWidget->mDump, SIGNAL(displayReferencesWidget()), this, SLOT(displayReferencesWidget()));
    connect(mCpuWidget->mStack, SIGNAL(displayReferencesWidget()), this, SLOT(displayReferencesWidget()));
    connect(Bridge::getBridge(), SIGNAL(getStrWindow(QString, QString*)), this, SLOT(getStrWindow(QString, QString*)));
    connect(Config(), SIGNAL(shortcutsUpdated()), this, SLOT(refreshShortcuts()));

    //Set default setttings (when not set)
    SettingsDialog defaultSettings;
    lastException = 0;
    defaultSettings.SaveSettings();

    //Create updatechecker
    mUpdateChecker = new UpdateChecker(this);

    //setup menu api
    initMenuApi();

    refreshShortcuts();

    bClose = false;
    mCloseDialog = new CloseDialog(this);

    mCpuWidget->mDisas->setFocus();
}

DWORD WINAPI MainWindow::closeThread(void* ptr)
{
    DbgExit();
    MainWindow* mainWindow = (MainWindow*)ptr;
    mainWindow->bClose = true;
    mainWindow->close();
    return 0;
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    hide(); //hide main window
    mCloseDialog->show();
    mCloseDialog->setFocus();
    static bool bExecuteThread = true;
    if(bExecuteThread)
    {
        bExecuteThread = false;
        CloseHandle(CreateThread(0, 0, closeThread, this, 0, 0));
    }
    if(bClose)
        event->accept();
    else
        event->ignore();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setTab(QWidget* widget)
{
    for(int i = 0; i < mTabWidget->count(); i++)
        if(mTabWidget->widget(i) == widget)
        {
            mTabWidget->setCurrentIndex(i);
            break;
        }
}

void MainWindow::refreshShortcuts()
{
    ui->actionOpen->setShortcut(ConfigShortcut("FileOpen"));
    ui->actionAttach->setShortcut(ConfigShortcut("FileAttach"));
    ui->actionDetach->setShortcut(ConfigShortcut("FileDetach"));
    ui->actionExit->setShortcut(ConfigShortcut("FileExit"));

    ui->actionCpu->setShortcut(ConfigShortcut("ViewCpu"));
    ui->actionLog->setShortcut(ConfigShortcut("ViewLog"));
    ui->actionBreakpoints->setShortcut(ConfigShortcut("ViewBreakpoints"));
    ui->actionMemoryMap->setShortcut(ConfigShortcut("ViewMemoryMap"));
    ui->actionCallStack->setShortcut(ConfigShortcut("ViewCallStack"));
    ui->actionScript->setShortcut(ConfigShortcut("ViewScript"));
    ui->actionSymbolInfo->setShortcut(ConfigShortcut("ViewSymbolInfo"));
    ui->actionReferences->setShortcut(ConfigShortcut("ViewReferences"));
    ui->actionThreads->setShortcut(ConfigShortcut("ViewThreads"));
    ui->actionPatches->setShortcut(ConfigShortcut("ViewPatches"));
    ui->actionComments->setShortcut(ConfigShortcut("ViewComments"));
    ui->actionLabels->setShortcut(ConfigShortcut("ViewLabels"));
    ui->actionBookmarks->setShortcut(ConfigShortcut("ViewBookmarks"));
    ui->actionFunctions->setShortcut(ConfigShortcut("ViewFunctions"));

    ui->actionRun->setShortcut(ConfigShortcut("DebugRun"));
    ui->actioneRun->setShortcut(ConfigShortcut("DebugeRun"));
    ui->actionRunSelection->setShortcut(ConfigShortcut("DebugRunSelection"));
    ui->actionPause->setShortcut(ConfigShortcut("DebugPause"));
    ui->actionRestart->setShortcut(ConfigShortcut("DebugRestart"));
    ui->actionClose->setShortcut(ConfigShortcut("DebugClose"));
    ui->actionStepInto->setShortcut(ConfigShortcut("DebugStepInto"));
    ui->actioneStepInto->setShortcut(ConfigShortcut("DebugeStepInfo"));
    ui->actionStepOver->setShortcut(ConfigShortcut("DebugStepOver"));
    ui->actioneStepOver->setShortcut(ConfigShortcut("DebugeStepOver"));
    ui->actionRtr->setShortcut(ConfigShortcut("DebugRtr"));
    ui->actioneRtr->setShortcut(ConfigShortcut("DebugeRtr"));
    ui->actionCommand->setShortcut(ConfigShortcut("DebugCommand"));
    ui->actionSkipNextInstruction->setShortcut(ConfigShortcut("DebugSkipNextInstruction"));

    ui->actionScylla->setShortcut(ConfigShortcut("PluginsScylla"));

    ui->actionSettings->setShortcut(ConfigShortcut("OptionsPreferences"));
    ui->actionAppearance->setShortcut(ConfigShortcut("OptionsAppearance"));
    ui->actionShortcuts->setShortcut(ConfigShortcut("OptionsShortcuts"));

    ui->actionAbout->setShortcut(ConfigShortcut("HelpAbout"));
    ui->actionDonate->setShortcut(ConfigShortcut("HelpDonate"));
    ui->actionCheckUpdates->setShortcut(ConfigShortcut("HelpCheckForUpdates"));
    ui->actionCalculator->setShortcut(ConfigShortcut("HelpCalculator"));

    ui->actionStrings->setShortcut(ConfigShortcut("ActionFindStrings"));
    ui->actionCalls->setShortcut(ConfigShortcut("ActionFindIntermodularCalls"));


}

//Reads recent files list from settings
void MainWindow::loadMRUList(int maxItems)
{
    mMaxMRU = maxItems;
    for(int i = 0; i < mMaxMRU; i++)
    {
        char currentFile[MAX_PATH] = "";
        if(!BridgeSettingGet("Recent Files", QString().sprintf("%.2d", i + 1).toUtf8().constData(), currentFile))
            break;
        if(QString(currentFile).size() && QFile(currentFile).exists())
            mMRUList.push_back(currentFile);
    }
    mMRUList.removeDuplicates();
    updateMRUMenu();
}

//save recent files to settings
void MainWindow::saveMRUList()
{
    BridgeSettingSet("Recent Files", 0, 0); //clear
    mMRUList.removeDuplicates();
    int mruSize = mMRUList.size();
    for(int i = 0; i < mruSize; i++)
    {
        if(QFile(mMRUList.at(i)).exists())
            BridgeSettingSet("Recent Files", QString().sprintf("%.2d", i + 1).toUtf8().constData(), mMRUList.at(i).toUtf8().constData());
    }
}

void MainWindow::addMRUEntry(QString entry)
{
    if(!entry.size())
        return;
    //remove duplicate entry if it exists
    removeMRUEntry(entry);
    mMRUList.insert(mMRUList.begin(), entry);
    if(mMRUList.size() > mMaxMRU)
        mMRUList.erase(mMRUList.begin() + mMaxMRU, mMRUList.end());
}

void MainWindow::removeMRUEntry(QString entry)
{
    if(!entry.size())
        return;
    QList<QString>::iterator it;

    for(it = mMRUList.begin(); it != mMRUList.end(); ++it)
    {
        if((*it) == entry)
        {
            mMRUList.erase(it);
            break;
        }
    }
}

void MainWindow::updateMRUMenu()
{
    if(mMaxMRU < 1) return;

    QMenu* fileMenu = this->menuBar()->findChild<QMenu*>(QString::fromWCharArray(L"menuFile"));
    if(fileMenu == NULL)
    {
        QMessageBox msg(QMessageBox::Critical, "Error!", "Failed to find menu!");
        msg.setWindowIcon(QIcon(":/icons/images/compile-error.png"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        msg.exec();
        return;
    }
    fileMenu = fileMenu->findChild<QMenu*>(QString::fromWCharArray(L"menuRecent_Files"));
    if(fileMenu == NULL)
    {
        QMessageBox msg(QMessageBox::Critical, "Error!", "Failed to find submenu!");
        msg.setWindowIcon(QIcon(":/icons/images/compile-error.png"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        msg.exec();
        return;
    }

    QList<QAction*> list = fileMenu->actions();
    for(int i = 1; i < list.length(); ++i)
        fileMenu->removeAction(list.at(i));

    //add items to list
    if(mMRUList.size() > 0)
    {
        list = fileMenu->actions();
        for(int index = 0; index < mMRUList.size(); ++index)
        {
            fileMenu->addAction(new QAction(mMRUList.at(index), this));
            fileMenu->actions().last()->setObjectName(QString("MRU").append(QString::number(index)));
            connect(fileMenu->actions().last(), SIGNAL(triggered()), this, SLOT(openFile()));
        }
    }
}

QString MainWindow::getMRUEntry(int index)
{
    QString path;

    if(index < mMRUList.size())
        path = mMRUList.at(index);

    return path;
}

void MainWindow::executeCommand()
{
    QString wCmd = mCmdLineEdit->text();

    DbgCmdExec(wCmd.toUtf8().constData());

    mCmdLineEdit->addLineToHistory(wCmd);
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
    mCmdLineEdit->setFocus();
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
    QString title = "About x64_dbg";
#else
    QString title = "About x32_dbg";
#endif
    title += QString().sprintf(" v%d", BridgeGetDbgVersion());
    QMessageBox msg(QMessageBox::Information, title, "Website:\nhttp://x64dbg.com\n\nAttribution:\nIcons8 (http://icons8.com)\nYusuke Kamiyamane (http://p.yusukekamiyamane.com)\n\nCompiled on:\n"__DATE__", "__TIME__);
    msg.setWindowIcon(QIcon(":/icons/images/information.png"));
    msg.setParent(this, Qt::Dialog);
    msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
    msg.exec();
}

void MainWindow::openFile()
{
    QString lastPath, filename;
    QAction* fileToOpen = qobject_cast<QAction*>(sender());

    //if sender is from recent list directly open file, otherwise show dialog
    if(fileToOpen == NULL || !fileToOpen->objectName().startsWith("MRU") || !(fileToOpen->text().length()))
    {
        lastPath = (mMRUList.size() > 0) ? mMRUList.at(0) : 0;
        filename = QFileDialog::getOpenFileName(this, tr("Open file"), lastPath, tr("Executables (*.exe *.dll);;All files (*.*)"));
        if(!filename.length())
            return;
        filename = QDir::toNativeSeparators(filename); //convert to native path format (with backlashes)
    }
    else
    {
        filename = fileToOpen->text();
    }
    DbgCmdExec(QString("init \"" + filename + "\"").toUtf8().constData());

    //file is from recent menu
    if(fileToOpen != NULL && fileToOpen->objectName().startsWith("MRU"))
    {
        fileToOpen->setObjectName(fileToOpen->objectName().split("U").at(1));
        int index = fileToOpen->objectName().toInt();

        QString exists = getMRUEntry(index);
        if(exists.length() == 0)
        {
            addMRUEntry(filename);
            updateMRUMenu();
            saveMRUList();
        }

        fileToOpen->setObjectName(fileToOpen->objectName().prepend("MRU"));
    }

    //file is from open button
    bool update = true;
    if(fileToOpen == NULL || fileToOpen->objectName().compare("actionOpen") == 0)
        for(int i = 0; i < mMRUList.size(); i++)
            if(mMRUList.at(i) == filename)
            {
                update = false;
                break;
            }
    if(update)
    {
        addMRUEntry(filename);
        updateMRUMenu();
        saveMRUList();
    }

    mCpuWidget->mDisas->setFocus();
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
    char filename[MAX_SETTING_SIZE] = "";
    if(!mMRUList.size())
        return;
    strcpy_s(filename, mMRUList.at(0).toUtf8().constData());
    if(DbgIsDebugging())
    {
        DbgCmdExec("stop"); //close current file (when present)
        Sleep(400);
    }
    DbgCmdExec(QString().sprintf("init \"%s\"", filename).toUtf8().constData());

    mCpuWidget->mDisas->setFocus();
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
        if(DbgIsDebugging())
            DbgCmdExecDirect("stop");
        QString cmd;
        DbgCmdExec(cmd.sprintf("init \"%s\"", filename.toUtf8().constData()).toUtf8().constData());
        pEvent->acceptProposedAction();
    }
}

void MainWindow::updateWindowTitleSlot(QString filename)
{
    if(filename.length())
        setWindowTitle(QString(mWindowMainTitle) + QString(" - ") + filename);
    else
        setWindowTitle(QString(mWindowMainTitle));
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

void MainWindow::execSkip()
{
    DbgCmdExec("skip");
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

void MainWindow::displayReferencesWidget()
{
    mReferenceManager->show();
    mReferenceManager->setFocus();
    setTab(mReferenceManager);
}

void MainWindow::displayThreadsWidget()
{
    mThreadView->show();
    mThreadView->setFocus();
    setTab(mThreadView);
}

void MainWindow::openSettings()
{
    SettingsDialog settings(this);
    settings.lastException = lastException;
    settings.exec();
}

void MainWindow::openAppearance()
{
    AppearanceDialog appearance(this);
    appearance.exec();
}

void MainWindow::openCalculator()
{
    mCalculatorDialog->showNormal();
    mCalculatorDialog->setFocus();
    mCalculatorDialog->setExpressionFocus();
}

void MainWindow::openShortcuts()
{
    ShortcutsDialog shortcuts(this);
    shortcuts.exec();
}

void MainWindow::addRecentFile(QString file)
{
    addMRUEntry(file);
    updateMRUMenu();
    saveMRUList();
}

void MainWindow::setLastException(unsigned int exceptionCode)
{
    lastException = exceptionCode;
}

void MainWindow::findStrings()
{
    DbgCmdExec(QString("strref " + QString("%1").arg(mCpuWidget->mDisas->rvaToVa(mCpuWidget->mDisas->getInitialSelection()), sizeof(int_t) * 2, 16, QChar('0')).toUpper()).toUtf8().constData());
    displayReferencesWidget();
}

void MainWindow::findModularCalls()
{
    DbgCmdExec(QString("modcallfind " + QString("%1").arg(mCpuWidget->mDisas->rvaToVa(mCpuWidget->mDisas->getInitialSelection()), sizeof(int_t) * 2, 16, QChar('0')).toUpper()).toUtf8().constData());
    displayReferencesWidget();
}

void MainWindow::addMenu(int hMenu, QString title)
{
    int nFound = -1;
    for(int i = 0; i < mMenuList.size(); i++)
    {
        if(hMenu == mMenuList.at(i).hMenu)
        {
            nFound = i;
            break;
        }
    }
    if(nFound == -1 && hMenu != -1)
    {
        Bridge::getBridge()->BridgeSetResult(-1);
        return;
    }
    MenuInfo newInfo;
    int hMenuNew = hMenuNext;
    hMenuNext++;
    QMenu* wMenu = new QMenu(title, this);
    newInfo.mMenu = wMenu;
    newInfo.hMenu = hMenuNew;
    newInfo.hParentMenu = hMenu;
    mMenuList.push_back(newInfo);
    if(hMenu == -1) //top-level
        ui->menuBar->addMenu(wMenu);
    else //deeper level
        mMenuList.at(nFound).mMenu->addMenu(wMenu);
    Bridge::getBridge()->BridgeSetResult(hMenuNew);
}

void MainWindow::addMenuEntry(int hMenu, QString title)
{
    int nFound = -1;
    for(int i = 0; i < mMenuList.size(); i++)
    {
        if(hMenu == mMenuList.at(i).hMenu)
        {
            nFound = i;
            break;
        }
    }
    if(nFound == -1 && hMenu != -1)
    {
        Bridge::getBridge()->BridgeSetResult(-1);
        return;
    }
    MenuEntryInfo newInfo;
    int hEntryNew = hEntryNext;
    hEntryNext++;
    newInfo.hEntry = hEntryNew;
    newInfo.hParentMenu = hMenu;
    QAction* wAction = new QAction(title, this);
    wAction->setObjectName(QString().sprintf("ENTRY|%d", hEntryNew));
    this->addAction(wAction);
    connect(wAction, SIGNAL(triggered()), this, SLOT(menuEntrySlot()));
    newInfo.mAction = wAction;
    mEntryList.push_back(newInfo);
    if(hMenu == -1) //top level
        ui->menuBar->addAction(wAction);
    else //deeper level
        mMenuList.at(nFound).mMenu->addAction(wAction);
    Bridge::getBridge()->BridgeSetResult(hEntryNew);
}

void MainWindow::addSeparator(int hMenu)
{
    int nFound = -1;
    for(int i = 0; i < mMenuList.size(); i++)
    {
        if(hMenu == mMenuList.at(i).hMenu) //we found a menu that has the menu as parent
        {
            nFound = i;
            break;
        }
    }
    if(nFound == -1) //not found
        return;
    MenuEntryInfo newInfo;
    newInfo.hEntry = -1;
    newInfo.hParentMenu = hMenu;
    newInfo.mAction = mMenuList.at(nFound).mMenu->addSeparator();
    mEntryList.push_back(newInfo);
}

void MainWindow::clearMenu(int hMenu)
{
    if(!mMenuList.size() || hMenu == -1)
        return;
    //delete menu entries
    for(int i = mEntryList.size() - 1; i > -1; i--)
    {
        if(hMenu == mEntryList.at(i).hParentMenu) //we found an entry that has the menu as parent
        {
            this->removeAction(mEntryList.at(i).mAction);
            delete mEntryList.at(i).mAction; //delete the entry object
            mEntryList.erase(mEntryList.begin() + i);
        }
    }
    //recursively delete the menus
    for(int i = mMenuList.size() - 1; i > -1; i--)
    {
        if(hMenu == mMenuList.at(i).hParentMenu) //we found a menu that has the menu as parent
        {
            clearMenu(mMenuList.at(i).hMenu); //delete children menus
            delete mMenuList.at(i).mMenu; //delete the child menu object
            mMenuList.erase(mMenuList.begin() + i); //delete the child entry
        }
    }
}

void MainWindow::initMenuApi()
{
    //256 entries are reserved
    mEntryList.clear();
    hEntryNext = 256;
    mMenuList.clear();
    hMenuNext = 256;
    MenuInfo newInfo;
    //add plugin menu
    newInfo.mMenu = ui->menuPlugins;
    newInfo.hMenu = GUI_PLUGIN_MENU;
    newInfo.hParentMenu = -1;
    mMenuList.push_back(newInfo);
}

void MainWindow::menuEntrySlot()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if(action && action->objectName().startsWith("ENTRY|"))
    {
        int hEntry = -1;
        if(sscanf_s(action->objectName().mid(6).toUtf8().constData(), "%d", &hEntry) == 1)
            DbgMenuEntryClicked(hEntry);
    }
}

void MainWindow::runSelection()
{
    if(!DbgIsDebugging())
        return;
    QString command = "bp " + QString("%1").arg(mCpuWidget->mDisas->rvaToVa(mCpuWidget->mDisas->getInitialSelection()), sizeof(int_t) * 2, 16, QChar('0')).toUpper() + ", ss";
    if(DbgCmdExecDirect(command.toUtf8().constData()))
        DbgCmdExecDirect("run");
}

void MainWindow::getStrWindow(const QString title, QString* text)
{
    LineEditDialog mLineEdit(this);
    mLineEdit.setWindowTitle(title);
    bool bResult = true;
    if(mLineEdit.exec() != QDialog::Accepted)
        bResult = false;
    *text = mLineEdit.editText;
    Bridge::getBridge()->BridgeSetResult(bResult);
}

void MainWindow::patchWindow()
{
    if(!DbgIsDebugging())
    {
        QMessageBox msg(QMessageBox::Critical, "Error!", QString("Patches cannot be shown when not debugging..."));
        msg.setWindowIcon(QIcon(":/icons/images/compile-error.png"));
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        msg.exec();
        return;
    }
    GuiUpdatePatches();
    mPatchDialog->showNormal();
    mPatchDialog->setFocus();
}

void MainWindow::displayComments()
{
    if(!DbgIsDebugging())
        return;
    DbgCmdExec("commentlist");
    displayReferencesWidget();
}

void MainWindow::displayLabels()
{
    if(!DbgIsDebugging())
        return;
    DbgCmdExec("labellist");
    displayReferencesWidget();
}

void MainWindow::displayBookmarks()
{
    if(!DbgIsDebugging())
        return;
    DbgCmdExec("bookmarklist");
    displayReferencesWidget();
}

void MainWindow::displayFunctions()
{
    if(!DbgIsDebugging())
        return;
    DbgCmdExec("functionlist");
    displayReferencesWidget();
}

void MainWindow::checkUpdates()
{
    mUpdateChecker->checkForUpdates();
}

void MainWindow::displayCallstack()
{
    mCallStackView->show();
    mCallStackView->setFocus();
    setTab(mCallStackView);
}

void MainWindow::donate()
{
    QMessageBox msg(QMessageBox::Information, "Donate", "All the money will go to x64_dbg development.");
    msg.setWindowIcon(QIcon(":/icons/images/donate.png"));
    msg.setParent(this, Qt::Dialog);
    msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
    msg.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msg.setDefaultButton(QMessageBox::Ok);
    if(msg.exec() != QMessageBox::Ok)
        return;
    QDesktopServices::openUrl(QUrl("https://blockchain.info/address/1GuXgtCrLk4aYgivAT7xAi8zVHWk5CkEoY"));
}

void MainWindow::displayAttach()
{
    AttachDialog attach(this);
    attach.exec();

    mCpuWidget->mDisas->setFocus();
}

void MainWindow::detach()
{
    DbgCmdExec("detach");
}

void MainWindow::changeCommandLine()
{
    if(!DbgIsDebugging())
        return;

    LineEditDialog mLineEdit(this);
    mLineEdit.setText("");
    mLineEdit.setWindowTitle("Change Command Line");
    mLineEdit.setWindowIcon(QIcon(":/icons/images/changeargs.png"));

    size_t cbsize = 0;
    char* cmdline = 0;
    if(!DbgFunctions()->GetCmdline(0, &cbsize))
        mLineEdit.setText("Cannot get remote command line, use the 'getcmdline'' command for more information.");
    else
    {
        cmdline = new char[cbsize];
        DbgFunctions()->GetCmdline(cmdline, 0);
        mLineEdit.setText(QString(cmdline));
        delete[] cmdline;
    }

    mLineEdit.setCursorPosition(0);

    if(mLineEdit.exec() != QDialog::Accepted)
        return; //pressed cancel

    if(!DbgFunctions()->SetCmdline((char*)mLineEdit.editText.toUtf8().constData()))
    {
        QMessageBox msg(QMessageBox::Warning, "Error", "Could not set command line!");
        msg.setWindowIcon(QIcon(":/icons/images/compile-warning.png"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        msg.exec();
    }
    else
    {
        DbgFunctions()->MemUpdateMap();
        GuiUpdateMemoryView();
        GuiAddStatusBarMessage(QString("New command line: " + mLineEdit.editText + "\n").toUtf8().constData());
    }
}
