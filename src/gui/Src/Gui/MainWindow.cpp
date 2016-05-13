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
#include "StringUtil.h"

QString MainWindow::windowTitle = "";

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Build information
    QAction* buildInfo = new QAction(ToDateString(GetCompileDate()), this);
    buildInfo->setEnabled(false);
    ui->menuBar->addAction(buildInfo);

    // Setup bridge signals
    connect(Bridge::getBridge(), SIGNAL(updateWindowTitle(QString)), this, SLOT(updateWindowTitleSlot(QString)));
    connect(Bridge::getBridge(), SIGNAL(addRecentFile(QString)), this, SLOT(addRecentFile(QString)));
    connect(Bridge::getBridge(), SIGNAL(setLastException(uint)), this, SLOT(setLastException(uint)));
    connect(Bridge::getBridge(), SIGNAL(menuAddMenuToList(QWidget*, QMenu*, int, int)), this, SLOT(addMenuToList(QWidget*, QMenu*, int, int)));
    connect(Bridge::getBridge(), SIGNAL(menuAddMenu(int, QString)), this, SLOT(addMenu(int, QString)));
    connect(Bridge::getBridge(), SIGNAL(menuAddMenuEntry(int, QString)), this, SLOT(addMenuEntry(int, QString)));
    connect(Bridge::getBridge(), SIGNAL(menuAddSeparator(int)), this, SLOT(addSeparator(int)));
    connect(Bridge::getBridge(), SIGNAL(menuClearMenu(int)), this, SLOT(clearMenu(int)));
    connect(Bridge::getBridge(), SIGNAL(menuRemoveMenuEntry(int)), this, SLOT(removeMenuEntry(int)));
    connect(Bridge::getBridge(), SIGNAL(getStrWindow(QString, QString*)), this, SLOT(getStrWindow(QString, QString*)));
    connect(Bridge::getBridge(), SIGNAL(setIconMenu(int, QIcon)), this, SLOT(setIconMenu(int, QIcon)));
    connect(Bridge::getBridge(), SIGNAL(setIconMenuEntry(int, QIcon)), this, SLOT(setIconMenuEntry(int, QIcon)));
    connect(Bridge::getBridge(), SIGNAL(showCpu()), this, SLOT(displayCpuWidget()));
    connect(Bridge::getBridge(), SIGNAL(addQWidgetTab(QWidget*)), this, SLOT(addQWidgetTab(QWidget*)));
    connect(Bridge::getBridge(), SIGNAL(showQWidgetTab(QWidget*)), this, SLOT(showQWidgetTab(QWidget*)));
    connect(Bridge::getBridge(), SIGNAL(closeQWidgetTab(QWidget*)), this, SLOT(closeQWidgetTab(QWidget*)));
    connect(Bridge::getBridge(), SIGNAL(executeOnGuiThread(void*)), this, SLOT(executeOnGuiThread(void*)));

    // Setup menu API
    initMenuApi();
    addMenuToList(this, ui->menuPlugins, GUI_PLUGIN_MENU);

    this->showMaximized();

#ifdef _WIN64
    mWindowMainTitle = "x64dbg";
#else
    mWindowMainTitle = "x32dbg";
#endif

    // Set window title
    setWindowTitle(QString(mWindowMainTitle));

    // Load application icon
    HICON hIcon = LoadIcon(GetModuleHandleA(0), MAKEINTRESOURCE(100));
    SendMessageA((HWND)MainWindow::winId(), WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    DestroyIcon(hIcon);

    // Load recent files
    loadMRUList(16);

    // Accept drops
    setAcceptDrops(true);

    // Log view
    mLogView = new LogView();
    mLogView->setWindowTitle("Log");
    mLogView->setWindowIcon(QIcon(":/icons/images/log.png"));
    mLogView->hide();

    // Symbol view
    mSymbolView = new SymbolView();
    mSymbolView->setWindowTitle("Symbols");
    mSymbolView->setWindowIcon(QIcon(":/icons/images/pdb.png"));
    mSymbolView->hide();

    // Source view
    mSourceViewManager = new SourceViewerManager();
    mSourceViewManager->setWindowTitle("Source");
    mSourceViewManager->setWindowIcon(QIcon(":/icons/images/source.png"));
    mSourceViewManager->hide();
    connect(mSourceViewManager, SIGNAL(showCpu()), this, SLOT(displayCpuWidget()));

    // Breakpoints
    mBreakpointsView = new BreakpointsView();
    mBreakpointsView->setWindowTitle("Breakpoints");
    mBreakpointsView->setWindowIcon(QIcon(":/icons/images/breakpoint.png"));
    mBreakpointsView->hide();
    connect(mBreakpointsView, SIGNAL(showCpu()), this, SLOT(displayCpuWidget()));

    // Memory map view
    mMemMapView = new MemoryMapView();
    connect(mMemMapView, SIGNAL(showCpu()), this, SLOT(displayCpuWidget()));
    connect(mMemMapView, SIGNAL(showReferences()), this, SLOT(displayReferencesWidget()));
    mMemMapView->setWindowTitle("Memory Map");
    mMemMapView->setWindowIcon(QIcon(":/icons/images/memory-map.png"));
    mMemMapView->hide();

    // Callstack view
    mCallStackView = new CallStackView();
    mCallStackView->setWindowTitle("Call Stack");
    mCallStackView->setWindowIcon(QIcon(":/icons/images/callstack.png"));
    connect(mCallStackView, SIGNAL(showCpu()), this, SLOT(displayCpuWidget()));

    // SEH Chain view
    mSEHChainView = new SEHChainView();
    mSEHChainView->setWindowTitle("SEH");
    mSEHChainView->setWindowIcon(QIcon(":/icons/images/seh-chain.png"));
    connect(mSEHChainView, SIGNAL(showCpu()), this, SLOT(displayCpuWidget()));

    // Script view
    mScriptView = new ScriptView();
    mScriptView->setWindowTitle("Script");
    mScriptView->setWindowIcon(QIcon(":/icons/images/script-code.png"));
    mScriptView->hide();

    // CPU view
    mCpuWidget = new CPUWidget();
    mCpuWidget->setWindowTitle("CPU");
#ifdef _WIN64
    mCpuWidget->setWindowIcon(QIcon(":/icons/images/processor64.png"));
#else
    mCpuWidget->setWindowIcon(QIcon(":/icons/images/processor32.png"));
    ui->actionCpu->setIcon(QIcon(":/icons/images/processor32.png"));
#endif //_WIN64

    // Reference manager
    mReferenceManager = new ReferenceManager(this);
    Bridge::getBridge()->referenceManager = mReferenceManager;
    mReferenceManager->setWindowTitle("References");
    mReferenceManager->setWindowIcon(QIcon(":/icons/images/search.png"));

    // Thread view
    mThreadView = new ThreadView();
    connect(mThreadView, SIGNAL(showCpu()), this, SLOT(displayCpuWidget()));
    mThreadView->setWindowTitle("Threads");
    mThreadView->setWindowIcon(QIcon(":/icons/images/arrow-threads.png"));

    // Snowman view (decompiler)
    mSnowmanView = CreateSnowman(this);
    mSnowmanView->setWindowTitle("Snowman");
    mSnowmanView->setWindowIcon(QIcon(":/icons/images/snowman.png"));

    // Notes manager
    mNotesManager = new NotesManager(this);
    mNotesManager->setWindowTitle("Notes");
    mNotesManager->setWindowIcon(QIcon(":/icons/images/notes.png"));

    // Create the tab widget
    mTabWidget = new MHTabWidget();

    // Add all widgets to the list
    mWidgetList.push_back(mCpuWidget);
    mWidgetList.push_back(mLogView);
    mWidgetList.push_back(mNotesManager);
    mWidgetList.push_back(mBreakpointsView);
    mWidgetList.push_back(mMemMapView);
    mWidgetList.push_back(mCallStackView);
    mWidgetList.push_back(mSEHChainView);
    mWidgetList.push_back(mScriptView);
    mWidgetList.push_back(mSymbolView);
    mWidgetList.push_back(mSourceViewManager);
    mWidgetList.push_back(mReferenceManager);
    mWidgetList.push_back(mThreadView);
    mWidgetList.push_back(mSnowmanView);

    // If LoadSaveTabOrder disabled, load tabs in default order
    if(!ConfigBool("Miscellaneous", "LoadSaveTabOrder"))
        loadTabDefaultOrder();
    else
        loadTabSavedOrder();

    setCentralWidget(mTabWidget);

    // Setup the command and status bars
    setupCommandBar();
    setupStatusBar();

    // Patch dialog
    mPatchDialog = new PatchDialog(this);
    mCalculatorDialog = new CalculatorDialog(this);
    connect(mCalculatorDialog, SIGNAL(showCpu()), this, SLOT(displayCpuWidget()));

    // Setup signals/slots
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
    connect(ui->actionSource, SIGNAL(triggered()), this, SLOT(displaySourceViewWidget()));
    connect(mSymbolView, SIGNAL(showCpu()), this, SLOT(displayCpuWidget()));
    connect(mSymbolView, SIGNAL(showReferences()), this, SLOT(displayReferencesWidget()));
    connect(mReferenceManager, SIGNAL(showCpu()), this, SLOT(displayCpuWidget()));
    connect(ui->actionReferences, SIGNAL(triggered()), this, SLOT(displayReferencesWidget()));
    connect(ui->actionThreads, SIGNAL(triggered()), this, SLOT(displayThreadsWidget()));
    connect(ui->actionSettings, SIGNAL(triggered()), this, SLOT(openSettings()));
    connect(ui->actionStrings, SIGNAL(triggered()), this, SLOT(findStrings()));
    connect(ui->actionCalls, SIGNAL(triggered()), this, SLOT(findModularCalls()));
    connect(ui->actionAppearance, SIGNAL(triggered()), this, SLOT(openAppearance()));
    connect(ui->actionShortcuts, SIGNAL(triggered()), this, SLOT(openShortcuts()));
    connect(ui->actionTopmost, SIGNAL(toggled(bool)), this, SLOT(changeTopmost(bool)));
    connect(ui->actionCalculator, SIGNAL(triggered()), this, SLOT(openCalculator()));
    connect(ui->actionPatches, SIGNAL(triggered()), this, SLOT(patchWindow()));
    connect(ui->actionComments, SIGNAL(triggered()), this, SLOT(displayComments()));
    connect(ui->actionLabels, SIGNAL(triggered()), this, SLOT(displayLabels()));
    connect(ui->actionBookmarks, SIGNAL(triggered()), this, SLOT(displayBookmarks()));
    connect(ui->actionFunctions, SIGNAL(triggered()), this, SLOT(displayFunctions()));
    connect(ui->actionCheckUpdates, SIGNAL(triggered()), this, SLOT(checkUpdates()));
    connect(ui->actionCallStack, SIGNAL(triggered()), this, SLOT(displayCallstack()));
    connect(ui->actionSEHChain, SIGNAL(triggered()), this, SLOT(displaySEHChain()));
    connect(ui->actionDonate, SIGNAL(triggered()), this, SLOT(donate()));
    connect(ui->actionReportBug, SIGNAL(triggered()), this, SLOT(reportBug()));
    connect(ui->actionAttach, SIGNAL(triggered()), this, SLOT(displayAttach()));
    connect(ui->actionDetach, SIGNAL(triggered()), this, SLOT(detach()));
    connect(ui->actionChangeCommandLine, SIGNAL(triggered()), this, SLOT(changeCommandLine()));
    connect(ui->actionManual, SIGNAL(triggered()), this, SLOT(displayManual()));

    connect(mCpuWidget->getDisasmWidget(), SIGNAL(updateWindowTitle(QString)), this, SLOT(updateWindowTitleSlot(QString)));
    connect(mCpuWidget->getDisasmWidget(), SIGNAL(displayReferencesWidget()), this, SLOT(displayReferencesWidget()));
    connect(mCpuWidget->getDisasmWidget(), SIGNAL(displaySourceManagerWidget()), this, SLOT(displaySourceViewWidget()));
    connect(mCpuWidget->getDisasmWidget(), SIGNAL(displaySnowmanWidget()), this, SLOT(displaySnowmanWidget()));
    connect(mCpuWidget->getDisasmWidget(), SIGNAL(showPatches()), this, SLOT(patchWindow()));
    connect(mCpuWidget->getDisasmWidget(), SIGNAL(decompileAt(dsint, dsint)), this, SLOT(decompileAt(dsint, dsint)));
    connect(mCpuWidget->getDumpWidget(), SIGNAL(displayReferencesWidget()), this, SLOT(displayReferencesWidget()));
    connect(mCpuWidget->getStackWidget(), SIGNAL(displayReferencesWidget()), this, SLOT(displayReferencesWidget()));
    connect(mTabWidget, SIGNAL(tabMovedTabWidget(int, int)), this, SLOT(tabMovedSlot(int, int)));
    connect(Config(), SIGNAL(shortcutsUpdated()), this, SLOT(refreshShortcuts()));


    // Set default setttings (when not set)
    SettingsDialog defaultSettings;
    lastException = 0;
    defaultSettings.SaveSettings();


    // Create updatechecker
    mUpdateChecker = new UpdateChecker(this);

    refreshShortcuts();

    // Setup close thread and dialog
    bCanClose = false;
    mCloseThread = new MainWindowCloseThread(this);
    connect(mCloseThread, SIGNAL(canClose()), this, SLOT(canClose()));
    mCloseDialog = new CloseDialog(this);

    mCpuWidget->setDisasmFocus();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupCommandBar()
{
    mCmdLineEdit = new CommandLineEdit(ui->cmdBar);
    ui->cmdBar->addWidget(new QLabel("Command: "));
    ui->cmdBar->addWidget(mCmdLineEdit);
    ui->cmdBar->addWidget(mCmdLineEdit->selectorWidget());
}

void MainWindow::setupStatusBar()
{
    // Status label (Ready, Paused, ...)
    mStatusLabel = new StatusLabel(ui->statusBar);
    mStatusLabel->setText("Ready");
    ui->statusBar->addWidget(mStatusLabel);

    // Log line
    mLastLogLabel = new StatusLabel();
    ui->statusBar->addPermanentWidget(mLastLogLabel, 1);

    // Time wasted counter
    QLabel* timeWastedLabel = new QLabel(this);
    ui->statusBar->addPermanentWidget(timeWastedLabel);
    mTimeWastedCounter = new TimeWastedCounter(this, timeWastedLabel);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    mCloseDialog->show();
    mCloseDialog->setFocus();
    static volatile bool bExecuteThread = true;
    if(bExecuteThread)
    {
        bExecuteThread = false;
        CloseSnowman(mSnowmanView);
        Sleep(100);
        mCloseThread->start();
    }
    if(bCanClose)
    {
        mCloseDialog->allowClose();
        mCloseDialog->close();
        event->accept();
    }
    else
        event->ignore();
}

void MainWindow::setTab(QWidget* widget)
{
    for(int i = 0; i < mTabWidget->count(); i++)
    {
        if(mTabWidget->widget(i) == widget)
        {
            mTabWidget->setCurrentIndex(i);
            break;
        }
    }
}

void MainWindow::loadTabDefaultOrder()
{
    clearTabWidget();

    // Setup tabs
    for(int i = 0; i < mWidgetList.size(); i++)
        addQWidgetTab(mWidgetList[i]);
}

void MainWindow::loadTabSavedOrder()
{
    clearTabWidget();

    QMap<duint, QWidget*> tabIndexToWidget;

    // Get tabIndex for each widget and add them to tabIndexToWidget
    for(int i = 0; i < mWidgetList.size(); i++)
    {
        QString tabName = mWidgetList[i]->windowTitle();
        tabName = tabName.replace(" ", "") + "Tab";
        duint tabIndex = Config()->getUint("TabOrder", tabName);
        tabIndexToWidget.insert(tabIndex, mWidgetList[i]);
    }

    // Setup tabs
    for(auto & widget : tabIndexToWidget)
        addQWidgetTab(widget);
}

void MainWindow::clearTabWidget()
{
    if(mTabWidget->count() <= 0)
        return;

    // Remove all tabs starting from the end
    for(int i = mTabWidget->count() - 1; i >= 0; i--)
        mTabWidget->removeTab(i);
}

void MainWindow::setGlobalShortcut(QAction* action, const QKeySequence & key)
{
    action->setShortcut(key);
    action->setShortcutContext(Qt::ApplicationShortcut);
}

void MainWindow::refreshShortcuts()
{
    setGlobalShortcut(ui->actionOpen, ConfigShortcut("FileOpen"));
    setGlobalShortcut(ui->actionAttach, ConfigShortcut("FileAttach"));
    setGlobalShortcut(ui->actionDetach, ConfigShortcut("FileDetach"));
    setGlobalShortcut(ui->actionExit, ConfigShortcut("FileExit"));

    setGlobalShortcut(ui->actionCpu, ConfigShortcut("ViewCpu"));
    setGlobalShortcut(ui->actionLog, ConfigShortcut("ViewLog"));
    setGlobalShortcut(ui->actionBreakpoints, ConfigShortcut("ViewBreakpoints"));
    setGlobalShortcut(ui->actionMemoryMap, ConfigShortcut("ViewMemoryMap"));
    setGlobalShortcut(ui->actionCallStack, ConfigShortcut("ViewCallStack"));
    setGlobalShortcut(ui->actionScript, ConfigShortcut("ViewScript"));
    setGlobalShortcut(ui->actionSymbolInfo, ConfigShortcut("ViewSymbolInfo"));
    setGlobalShortcut(ui->actionSource, ConfigShortcut("ViewSource"));
    setGlobalShortcut(ui->actionReferences, ConfigShortcut("ViewReferences"));
    setGlobalShortcut(ui->actionThreads, ConfigShortcut("ViewThreads"));
    setGlobalShortcut(ui->actionPatches, ConfigShortcut("ViewPatches"));
    setGlobalShortcut(ui->actionComments, ConfigShortcut("ViewComments"));
    setGlobalShortcut(ui->actionLabels, ConfigShortcut("ViewLabels"));
    setGlobalShortcut(ui->actionBookmarks, ConfigShortcut("ViewBookmarks"));
    setGlobalShortcut(ui->actionFunctions, ConfigShortcut("ViewFunctions"));

    setGlobalShortcut(ui->actionRun, ConfigShortcut("DebugRun"));
    setGlobalShortcut(ui->actioneRun, ConfigShortcut("DebugeRun"));
    setGlobalShortcut(ui->actionRunSelection, ConfigShortcut("DebugRunSelection"));
    setGlobalShortcut(ui->actionPause, ConfigShortcut("DebugPause"));
    setGlobalShortcut(ui->actionRestart, ConfigShortcut("DebugRestart"));
    setGlobalShortcut(ui->actionClose, ConfigShortcut("DebugClose"));
    setGlobalShortcut(ui->actionStepInto, ConfigShortcut("DebugStepInto"));
    setGlobalShortcut(ui->actioneStepInto, ConfigShortcut("DebugeStepInfo"));
    setGlobalShortcut(ui->actionStepOver, ConfigShortcut("DebugStepOver"));
    setGlobalShortcut(ui->actioneStepOver, ConfigShortcut("DebugeStepOver"));
    setGlobalShortcut(ui->actionRtr, ConfigShortcut("DebugRtr"));
    setGlobalShortcut(ui->actioneRtr, ConfigShortcut("DebugeRtr"));
    setGlobalShortcut(ui->actionCommand, ConfigShortcut("DebugCommand"));
    setGlobalShortcut(ui->actionSkipNextInstruction, ConfigShortcut("DebugSkipNextInstruction"));

    setGlobalShortcut(ui->actionScylla, ConfigShortcut("PluginsScylla"));

    setGlobalShortcut(ui->actionSettings, ConfigShortcut("OptionsPreferences"));
    setGlobalShortcut(ui->actionAppearance, ConfigShortcut("OptionsAppearance"));
    setGlobalShortcut(ui->actionShortcuts, ConfigShortcut("OptionsShortcuts"));
    setGlobalShortcut(ui->actionTopmost, ConfigShortcut("OptionsTopmost"));

    setGlobalShortcut(ui->actionAbout, ConfigShortcut("HelpAbout"));
    setGlobalShortcut(ui->actionDonate, ConfigShortcut("HelpDonate"));
    setGlobalShortcut(ui->actionCheckUpdates, ConfigShortcut("HelpCheckForUpdates"));
    setGlobalShortcut(ui->actionCalculator, ConfigShortcut("HelpCalculator"));
    setGlobalShortcut(ui->actionReportBug, ConfigShortcut("HelpReportBug"));
    setGlobalShortcut(ui->actionManual, ConfigShortcut("HelpManual"));

    setGlobalShortcut(ui->actionStrings, ConfigShortcut("ActionFindStrings"));
    setGlobalShortcut(ui->actionCalls, ConfigShortcut("ActionFindIntermodularCalls"));
}

//Reads recent files list from settings
void MainWindow::loadMRUList(int maxItems)
{
    mMaxMRU = maxItems;
    for(int i = 0; i < mMaxMRU; i++)
    {
        char currentFile[MAX_SETTING_SIZE] = "";
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
    if(mMaxMRU < 1)
        return;

    QMenu* fileMenu = ui->menuRecentFiles;
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
    if(index < mMRUList.size())
        return mMRUList.at(index);

    return "";
}

void MainWindow::executeCommand()
{
    mCmdLineEdit->execute();
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
    showQWidgetTab(mMemMapView);
}

void MainWindow::displayLogWidget()
{
    showQWidgetTab(mLogView);
}

void MainWindow::displayScriptWidget()
{
    showQWidgetTab(mScriptView);
}

void MainWindow::displayAboutWidget()
{
#ifdef _WIN64
    QString title = "About x64dbg";
#else
    QString title = "About x32dbg";
#endif
    title += QString().sprintf(" v%d", BridgeGetDbgVersion());
    QMessageBox msg(QMessageBox::Information, title, "Website:<br><a href=\"http://x64dbg.com\">http://x64dbg.com</a><br><br>Attribution:<br><a href=\"http://icons8.com\">Icons8</a><br><a href=\"http://p.yusukekamiyamane.com\">Yusuke Kamiyamane</a><br><br>Compiled on:<br>" + ToDateString(GetCompileDate()) + ", " __TIME__);
    msg.setWindowIcon(QIcon(":/icons/images/information.png"));
    msg.setTextFormat(Qt::RichText);
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
    DbgCmdExec(QString().sprintf("init \"%s\"", filename.toUtf8().constData()).toUtf8().constData());

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

    mCpuWidget->setDisasmFocus();
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
    if(!mMRUList.size())
        return;
    DbgCmdExec(QString().sprintf("init \"%s\"", mMRUList.at(0).toUtf8().constData()).toUtf8().constData());

    mCpuWidget->setDisasmFocus();
}

void MainWindow::displayBreakpointWidget()
{
    showQWidgetTab(mBreakpointsView);
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
        DbgCmdExec(QString().sprintf("init \"%s\"", filename.toUtf8().constData()).toUtf8().constData());
        pEvent->acceptProposedAction();
    }
}

void MainWindow::updateWindowTitleSlot(QString filename)
{
    if(filename.length())
    {
        setWindowTitle(QString(mWindowMainTitle) + QString(" - ") + filename);
        windowTitle = filename;
    }
    else
    {
        setWindowTitle(QString(mWindowMainTitle));
        windowTitle = QString(mWindowMainTitle);
    }
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
    showQWidgetTab(mCpuWidget);
}

void MainWindow::displaySymbolWidget()
{
    showQWidgetTab(mSymbolView);
}

void MainWindow::displaySourceViewWidget()
{
    showQWidgetTab(mSourceViewManager);
}

void MainWindow::displayReferencesWidget()
{
    showQWidgetTab(mReferenceManager);
}

void MainWindow::displayThreadsWidget()
{
    showQWidgetTab(mThreadView);
}

void MainWindow::displaySnowmanWidget()
{
    showQWidgetTab(mSnowmanView);
}

void MainWindow::openSettings()
{
    SettingsDialog* settings = new SettingsDialog(this);
    connect(settings, SIGNAL(chkSaveLoadTabOrderStateChanged(bool)), this, SLOT(chkSaveloadTabSavedOrderStateChangedSlot(bool)));
    settings->lastException = lastException;
    settings->exec();
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

void MainWindow::changeTopmost(bool checked)
{
    if(checked)
        SetWindowPos((HWND)this->winId(), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    else
        SetWindowPos((HWND)this->winId(), HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
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
    DbgCmdExec(QString("strref " + QString("%1").arg(mCpuWidget->getDisasmWidget()->getSelectedVa(), sizeof(dsint) * 2, 16, QChar('0')).toUpper()).toUtf8().constData());
    displayReferencesWidget();
}

void MainWindow::findModularCalls()
{
    DbgCmdExec(QString("modcallfind " + QString("%1").arg(mCpuWidget->getDisasmWidget()->getSelectedVa(), sizeof(dsint) * 2, 16, QChar('0')).toUpper()).toUtf8().constData());
    displayReferencesWidget();
}

const MainWindow::MenuInfo* MainWindow::findMenu(int hMenu)
{
    if(hMenu == -1)
        return 0;
    int nFound = -1;
    for(int i = 0; i < mMenuList.size(); i++)
    {
        if(hMenu == mMenuList.at(i).hMenu)
        {
            nFound = i;
            break;
        }
    }
    return nFound == -1 ? 0 : &mMenuList.at(nFound);
}

void MainWindow::addMenuToList(QWidget* parent, QMenu* menu, int hMenu, int hParentMenu)
{
    if(!findMenu(hMenu))
        mMenuList.push_back(MenuInfo(parent, menu, hMenu, hParentMenu));
    Bridge::getBridge()->setResult();
}

void MainWindow::addMenu(int hMenu, QString title)
{
    const MenuInfo* menu = findMenu(hMenu);
    if(!menu && hMenu != -1)
    {
        Bridge::getBridge()->setResult(-1);
        return;
    }
    int hMenuNew = hMenuNext++;
    QWidget* parent = hMenu == -1 ? this : menu->parent;
    QMenu* wMenu = new QMenu(title, parent);
    wMenu->menuAction()->setVisible(false);
    mMenuList.push_back(MenuInfo(parent, wMenu, hMenuNew, hMenu));
    if(hMenu == -1) //top-level
        ui->menuBar->addMenu(wMenu);
    else //deeper level
        menu->mMenu->addMenu(wMenu);
    Bridge::getBridge()->setResult(hMenuNew);
}

void MainWindow::addMenuEntry(int hMenu, QString title)
{
    const MenuInfo* menu = findMenu(hMenu);
    if(!menu && hMenu != -1)
    {
        Bridge::getBridge()->setResult(-1);
        return;
    }
    MenuEntryInfo newInfo;
    int hEntryNew = hEntryNext++;
    newInfo.hEntry = hEntryNew;
    newInfo.hParentMenu = hMenu;
    QWidget* parent = hMenu == -1 ? this : menu->parent;
    QAction* wAction = new QAction(title, parent);
    wAction->setObjectName(QString().sprintf("ENTRY|%d", hEntryNew));
    parent->addAction(wAction);
    connect(wAction, SIGNAL(triggered()), this, SLOT(menuEntrySlot()));
    newInfo.mAction = wAction;
    mEntryList.push_back(newInfo);
    if(hMenu == -1) //top level
        ui->menuBar->addAction(wAction);
    else //deeper level
    {
        menu->mMenu->addAction(wAction);
        menu->mMenu->menuAction()->setVisible(true);
    }
    Bridge::getBridge()->setResult(hEntryNew);
}

void MainWindow::addSeparator(int hMenu)
{
    const MenuInfo* menu = findMenu(hMenu);
    if(menu)
    {
        MenuEntryInfo newInfo;
        newInfo.hEntry = -1;
        newInfo.hParentMenu = hMenu;
        newInfo.mAction = menu->mMenu->addSeparator();
        mEntryList.push_back(newInfo);
    }
    Bridge::getBridge()->setResult();
}

void MainWindow::clearMenu(int hMenu)
{
    if(!mMenuList.size() || hMenu == -1)
    {
        Bridge::getBridge()->setResult();
        return;
    }
    const MenuInfo* menu = findMenu(hMenu);
    //delete menu entries
    for(int i = mEntryList.size() - 1; i > -1; i--)
    {
        if(hMenu == mEntryList.at(i).hParentMenu) //we found an entry that has the menu as parent
        {
            QWidget* parent = menu == 0 ? this : menu->parent;
            parent->removeAction(mEntryList.at(i).mAction);
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
    //hide the empty menu
    if(menu)
        menu->mMenu->menuAction()->setVisible(false);
    Bridge::getBridge()->setResult();
}

void MainWindow::initMenuApi()
{
    //256 entries are reserved
    mEntryList.clear();
    hEntryNext = 256;
    mMenuList.clear();
    hMenuNext = 256;
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

void MainWindow::removeMenuEntry(int hEntry)
{
    for(int i = 0; i < mEntryList.size(); i++)
    {
        if(mEntryList.at(i).hEntry == hEntry)
        {
            const MenuEntryInfo & entry = mEntryList.at(i);
            const MenuInfo* menu = findMenu(entry.hParentMenu);
            QWidget* parent = menu == 0 ? this : menu->parent;
            parent->removeAction(entry.mAction);
            delete entry.mAction;
            mEntryList.erase(mEntryList.begin() + i);
            break;
        }
    }
    Bridge::getBridge()->setResult();
}

void MainWindow::setIconMenuEntry(int hEntry, QIcon icon)
{
    for(int i = 0; i < mEntryList.size(); i++)
    {
        if(mEntryList.at(i).hEntry == hEntry)
        {
            const MenuEntryInfo & entry = mEntryList.at(i);
            entry.mAction->setIcon(icon);
            break;
        }
    }
    Bridge::getBridge()->setResult();
}

void MainWindow::setIconMenu(int hMenu, QIcon icon)
{
    for(int i = 0; i < mMenuList.size(); i++)
    {
        if(mMenuList.at(i).hMenu == hMenu)
        {
            const MenuInfo & menu = mMenuList.at(i);
            menu.mMenu->setIcon(icon);
        }
    }
    Bridge::getBridge()->setResult();
}

void MainWindow::runSelection()
{
    if(!DbgIsDebugging())
        return;

    QString command = "bp " + QString("%1").arg(mCpuWidget->getDisasmWidget()->getSelectedVa(), sizeof(dsint) * 2, 16, QChar('0')).toUpper() + ", ss";
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
    Bridge::getBridge()->setResult(bResult);
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
    showQWidgetTab(mCallStackView);
}

void MainWindow::displaySEHChain()
{
    showQWidgetTab(mSEHChainView);
}

void MainWindow::donate()
{
    QMessageBox msg(QMessageBox::Information, "Donate", "All the money will go to x64dbg development.");
    msg.setWindowIcon(QIcon(":/icons/images/donate.png"));
    msg.setParent(this, Qt::Dialog);
    msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
    msg.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msg.setDefaultButton(QMessageBox::Ok);
    if(msg.exec() != QMessageBox::Ok)
        return;
    QDesktopServices::openUrl(QUrl("http://donate.x64dbg.com"));
}

void MainWindow::reportBug()
{
    QMessageBox msg(QMessageBox::Information, "Report Bug", "You will be taken to a website where you can report a bug.\nMake sure to fill in as much information as possible.");
    msg.setWindowIcon(QIcon(":/icons/images/bug-report.png"));
    msg.setParent(this, Qt::Dialog);
    msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
    msg.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msg.setDefaultButton(QMessageBox::Ok);
    if(msg.exec() != QMessageBox::Ok)
        return;
    QDesktopServices::openUrl(QUrl("http://report.x64dbg.com"));
}

void MainWindow::displayAttach()
{
    AttachDialog attach(this);
    attach.exec();

    mCpuWidget->setDisasmFocus();
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
        mLineEdit.setText("Cannot get remote command line, use the 'getcmdline' command for more information.");
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

void MainWindow::displayManual()
{
    // Open the Windows CHM in the upper directory
    QDesktopServices::openUrl(QUrl("..\\x64dbg.chm"));
}

void MainWindow::decompileAt(dsint start, dsint end)
{
    DecompileAt(mSnowmanView, start, end);
}

void MainWindow::canClose()
{
    bCanClose = true;
    close();
}

void MainWindow::addQWidgetTab(QWidget* qWidget)
{
    mTabWidget->addTab(qWidget, qWidget->windowIcon(), qWidget->windowTitle());
}

void MainWindow::showQWidgetTab(QWidget* qWidget)
{
    qWidget->show();
    qWidget->setFocus();
    setTab(qWidget);
}

void MainWindow::closeQWidgetTab(QWidget* qWidget)
{
    for(int i = 0; i < mTabWidget->count(); i++)
    {
        if(mTabWidget->widget(i) == qWidget)
        {
            mTabWidget->DeleteTab(i);
            break;
        }
    }
}

void MainWindow::executeOnGuiThread(void* cbGuiThread)
{
    ((GUICALLBACK)cbGuiThread)();
}

void MainWindow::tabMovedSlot(int from, int to)
{
    for(int i = 0; i < mTabWidget->count(); i++)
    {
        // Remove space in widget name and append Tab to get config settings (CPUTab, MemoryMapTab, etc...)
        QString tabName = mTabWidget->tabText(i).replace(" ", "") + "Tab";
        Config()->setUint("TabOrder", tabName, i);
    }
}

void MainWindow::chkSaveloadTabSavedOrderStateChangedSlot(bool state)
{
    if(state)
        loadTabSavedOrder();
    else
        loadTabDefaultOrder();
}

void MainWindow::on_actionFaq_triggered()
{
    QDesktopServices::openUrl(QUrl("http://faq.x64dbg.com"));
}
