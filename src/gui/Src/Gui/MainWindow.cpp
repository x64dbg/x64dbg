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
#include "MiscUtil.h"
#include "FavouriteTools.h"
#include "CPUDisassembly.h"
#include "CloseDialog.h"
#include <QVBoxLayout>
#include "CommandLineEdit.h"
#include "TabWidget.h"
#include "CPUWidget.h"
#include "MemoryMapView.h"
#include "CallStackView.h"
#include "SEHChainView.h"
#include "LogView.h"
#include "SymbolView.h"
#include "BreakpointsView.h"
#include "ScriptView.h"
#include "ReferenceManager.h"
#include "ThreadView.h"
#include "PatchDialog.h"
#include "CalculatorDialog.h"
#include "DebugStatusLabel.h"
#include "LogStatusLabel.h"
#include "SourceViewerManager.h"
#include "SnowmanView.h"
#include "HandlesView.h"
#include "MainWindowCloseThread.h"
#include "TimeWastedCounter.h"
#include "NotesManager.h"
#include "SettingsDialog.h"
#include "DisassemblerGraphViewOld.h"
#include "CPUMultiDump.h"
#include "CPUStack.h"
#include "GotoDialog.h"
#include "BrowseDialog.h"
#include "CustomizeMenuDialog.h"
#include "main.h"
#include "SimpleTraceDialog.h"
#include "CPUArgumentWidget.h"
#include "MRUList.h"
#include "AboutDialog.h"
#include "UpdateChecker.h"
#include "Tracer/TraceBrowser.h"

QString MainWindow::windowTitle = "";

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow)
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
    connect(Bridge::getBridge(), SIGNAL(menuClearMenu(int, bool)), this, SLOT(clearMenu(int, bool)));
    connect(Bridge::getBridge(), SIGNAL(menuRemoveMenuEntry(int)), this, SLOT(removeMenuEntry(int)));
    connect(Bridge::getBridge(), SIGNAL(getStrWindow(QString, QString*)), this, SLOT(getStrWindow(QString, QString*)));
    connect(Bridge::getBridge(), SIGNAL(setIconMenu(int, QIcon)), this, SLOT(setIconMenu(int, QIcon)));
    connect(Bridge::getBridge(), SIGNAL(setIconMenuEntry(int, QIcon)), this, SLOT(setIconMenuEntry(int, QIcon)));
    connect(Bridge::getBridge(), SIGNAL(setCheckedMenuEntry(int, bool)), this, SLOT(setCheckedMenuEntry(int, bool)));
    connect(Bridge::getBridge(), SIGNAL(setHotkeyMenuEntry(int, QString, QString)), this, SLOT(setHotkeyMenuEntry(int, QString, QString)));
    connect(Bridge::getBridge(), SIGNAL(showCpu()), this, SLOT(displayCpuWidget()));
    connect(Bridge::getBridge(), SIGNAL(addQWidgetTab(QWidget*)), this, SLOT(addQWidgetTab(QWidget*)));
    connect(Bridge::getBridge(), SIGNAL(showQWidgetTab(QWidget*)), this, SLOT(showQWidgetTab(QWidget*)));
    connect(Bridge::getBridge(), SIGNAL(closeQWidgetTab(QWidget*)), this, SLOT(closeQWidgetTab(QWidget*)));
    connect(Bridge::getBridge(), SIGNAL(executeOnGuiThread(void*)), this, SLOT(executeOnGuiThread(void*)));
    connect(Bridge::getBridge(), SIGNAL(dbgStateChanged(DBGSTATE)), this, SLOT(dbgStateChangedSlot(DBGSTATE)));
    connect(Bridge::getBridge(), SIGNAL(addFavouriteItem(int, QString, QString)), this, SLOT(addFavouriteItem(int, QString, QString)));
    connect(Bridge::getBridge(), SIGNAL(setFavouriteItemShortcut(int, QString, QString)), this, SLOT(setFavouriteItemShortcut(int, QString, QString)));
    connect(Bridge::getBridge(), SIGNAL(selectInMemoryMap(duint)), this, SLOT(displayMemMapWidget()));
    connect(Bridge::getBridge(), SIGNAL(closeApplication()), this, SLOT(close()));

    // Setup menu API
    initMenuApi();
    addMenuToList(this, ui->menuPlugins, GUI_PLUGIN_MENU);

    // Set window title
    mWindowMainTitle = QCoreApplication::applicationName();
    setWindowTitle(QString(mWindowMainTitle));

    // Load application icon
    SetApplicationIcon(MainWindow::winId());

    // Load recent files
    mMRUList = new MRUList(this, "Recent Files");
    connect(mMRUList, SIGNAL(openFile(QString)), this, SLOT(openRecentFileSlot(QString)));
    mMRUList->load();
    updateMRUMenu();

    // Accept drops
    setAcceptDrops(true);

    // Log view
    mLogView = new LogView();
    mLogView->setWindowTitle(tr("Log"));
    mLogView->setWindowIcon(DIcon("log.png"));
    mLogView->hide();

    // Symbol view
    mSymbolView = new SymbolView();
    mSymbolView->setWindowTitle(tr("Symbols"));
    mSymbolView->setWindowIcon(DIcon("pdb.png"));
    mSymbolView->hide();

    // Source view
    mSourceViewManager = new SourceViewerManager();
    mSourceViewManager->setWindowTitle(tr("Source"));
    mSourceViewManager->setWindowIcon(DIcon("source.png"));
    mSourceViewManager->hide();

    // Breakpoints
    mBreakpointsView = new BreakpointsView();
    mBreakpointsView->setWindowTitle(tr("Breakpoints"));
    mBreakpointsView->setWindowIcon(DIcon("breakpoint.png"));
    mBreakpointsView->hide();

    // Memory map view
    mMemMapView = new MemoryMapView();

    connect(mMemMapView, SIGNAL(showReferences()), this, SLOT(displayReferencesWidget()));
    mMemMapView->setWindowTitle(tr("Memory Map"));
    mMemMapView->setWindowIcon(DIcon("memory-map.png"));
    mMemMapView->hide();

    // Callstack view
    mCallStackView = new CallStackView();
    mCallStackView->setWindowTitle(tr("Call Stack"));
    mCallStackView->setWindowIcon(DIcon("callstack.png"));

    // SEH Chain view
    mSEHChainView = new SEHChainView();
    mSEHChainView->setWindowTitle(tr("SEH"));
    mSEHChainView->setWindowIcon(DIcon("seh-chain.png"));

    // Script view
    mScriptView = new ScriptView();
    mScriptView->setWindowTitle(tr("Script"));
    mScriptView->setWindowIcon(DIcon("script-code.png"));
    mScriptView->hide();

    // CPU view
    mCpuWidget = new CPUWidget();
    mCpuWidget->setWindowTitle(tr("CPU"));
#ifdef _WIN64
    mCpuWidget->setWindowIcon(DIcon("processor64.png"));
#else
    mCpuWidget->setWindowIcon(DIcon("processor32.png"));
    ui->actionCpu->setIcon(DIcon("processor32.png"));
#endif //_WIN64

    // Reference manager
    mReferenceManager = new ReferenceManager(this);
    Bridge::getBridge()->referenceManager = mReferenceManager;
    mReferenceManager->setWindowTitle(tr("References"));
    mReferenceManager->setWindowIcon(DIcon("search.png"));

    // Thread view
    mThreadView = new ThreadView();
    mThreadView->setWindowTitle(tr("Threads"));
    mThreadView->setWindowIcon(DIcon("arrow-threads.png"));

    // Snowman view (decompiler)
    mSnowmanView = CreateSnowman(this);
    if(!mSnowmanView)
        mSnowmanView = new QLabel("<center>Snowman is disabled...</center>", this);
    mSnowmanView->setWindowTitle(tr("Snowman"));
    mSnowmanView->setWindowIcon(DIcon("snowman.png"));
    Bridge::getBridge()->snowmanView = mSnowmanView;

    // Notes manager
    mNotesManager = new NotesManager(this);
    mNotesManager->setWindowTitle(tr("Notes"));
    mNotesManager->setWindowIcon(DIcon("notes.png"));

    // Handles view
    mHandlesView = new HandlesView(this);
    mHandlesView->setWindowTitle(tr("Handles"));
    mHandlesView->setWindowIcon(DIcon("handles.png"));

    // Graph view
    mGraphView = new DisassemblerGraphViewOld(this);
    mGraphView->setWindowTitle(tr("Graph"));
    mGraphView->setWindowIcon(DIcon("graph.png"));

    // Trace view
    mTraceBrowser = new TraceBrowser(this);
    mTraceBrowser->setWindowTitle(tr("Trace"));
    mTraceBrowser->setWindowIcon(DIcon("trace.png"));
    connect(mTraceBrowser, SIGNAL(displayReferencesWidget()), this, SLOT(displayReferencesWidget()));

    mTabWidget = new MHTabWidget(this, true, true);

    // Add all widgets to the list
    mWidgetList.push_back(WidgetInfo(mCpuWidget, "CPUTab"));
    mWidgetList.push_back(WidgetInfo(mGraphView, "GraphTab"));
    mWidgetList.push_back(WidgetInfo(mLogView, "LogTab"));
    mWidgetList.push_back(WidgetInfo(mNotesManager, "NotesTab"));
    mWidgetList.push_back(WidgetInfo(mBreakpointsView, "BreakpointsTab"));
    mWidgetList.push_back(WidgetInfo(mMemMapView, "MemoryMapTab"));
    //mWidgetList.push_back(WidgetInfo(mCallStackView, "CallStackTab"));
    mWidgetList.push_back(WidgetInfo(mSEHChainView, "SEHTab"));
    mWidgetList.push_back(WidgetInfo(mScriptView, "ScriptTab"));
    mWidgetList.push_back(WidgetInfo(mSymbolView, "SymbolsTab"));
    mWidgetList.push_back(WidgetInfo(mSourceViewManager, "SourceTab"));
    mWidgetList.push_back(WidgetInfo(mReferenceManager, "ReferencesTab"));
    QVBoxLayout* x = new QVBoxLayout();
    x->addWidget(mThreadView);
    x->addWidget(mCallStackView);
    QWidget* w = new QWidget();
    w->setLayout(x);
    mWidgetList.push_back(WidgetInfo(w, "ThreadsTab"));
    mWidgetList.push_back(WidgetInfo(mSnowmanView, "SnowmanTab"));
    mWidgetList.push_back(WidgetInfo(mHandlesView, "HandlesTab"));
    mWidgetList.push_back(WidgetInfo(mTraceBrowser, "TraceTab"));

    // If LoadSaveTabOrder disabled, load tabs in default order
    if(!ConfigBool("Gui", "LoadSaveTabOrder"))
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

    // Setup signals/slots
    connect(mCmdLineEdit, SIGNAL(returnPressed()), this, SLOT(executeCommand()));
    makeCommandAction(ui->actionStepOver, "StepOver");
    makeCommandAction(ui->actionStepInto, "StepInto");
    connect(ui->actionCommand, SIGNAL(triggered()), this, SLOT(setFocusToCommandBar()));
    makeCommandAction(ui->actionClose, "stop");
    connect(ui->actionMemoryMap, SIGNAL(triggered()), this, SLOT(displayMemMapWidget()));
    connect(ui->actionRun, SIGNAL(triggered()), this, SLOT(runSlot()));
    makeCommandAction(ui->actionRtr, "rtr");
    connect(ui->actionLog, SIGNAL(triggered()), this, SLOT(displayLogWidget()));
    connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(displayAboutWidget()));
    connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(openFileSlot()));
    makeCommandAction(ui->actionPause, "pause");
    makeCommandAction(ui->actionScylla, "StartScylla");
    connect(ui->actionRestart, SIGNAL(triggered()), this, SLOT(restartDebugging()));
    connect(ui->actionBreakpoints, SIGNAL(triggered()), this, SLOT(displayBreakpointWidget()));
    makeCommandAction(ui->actioneStepOver, "eStepOver");
    makeCommandAction(ui->actioneStepInto, "eStepInto");
    makeCommandAction(ui->actioneRun, "eRun");
    makeCommandAction(ui->actioneRtr, "eRtr");
    makeCommandAction(ui->actionRtu, "TraceOverConditional mod.user(cip)");
    connect(ui->actionTicnd, SIGNAL(triggered()), this, SLOT(execTicnd()));
    connect(ui->actionTocnd, SIGNAL(triggered()), this, SLOT(execTocnd()));
    connect(ui->actionTRBit, SIGNAL(triggered()), this, SLOT(execTRBit()));
    connect(ui->actionTRByte, SIGNAL(triggered()), this, SLOT(execTRByte()));
    connect(ui->actionTRWord, SIGNAL(triggered()), this, SLOT(execTRWord()));
    connect(ui->actionTRNone, SIGNAL(triggered()), this, SLOT(execTRNone()));
    makeCommandAction(ui->actionTRTIBT, "tibt");
    makeCommandAction(ui->actionTRTOBT, "tobt");
    makeCommandAction(ui->actionTRTIIT, "tiit");
    makeCommandAction(ui->actionTRTOIT, "toit");
    makeCommandAction(ui->actionInstrUndo, "InstrUndo");
    makeCommandAction(ui->actionSkipNextInstruction, "skip");
    connect(ui->actionScript, SIGNAL(triggered()), this, SLOT(displayScriptWidget()));
    connect(ui->actionRunSelection, SIGNAL(triggered()), this, SLOT(runSelection()));
    connect(ui->actionRunExpression, SIGNAL(triggered(bool)), this, SLOT(runExpression()));
    makeCommandAction(ui->actionHideDebugger, "hide");
    connect(ui->actionCpu, SIGNAL(triggered()), this, SLOT(displayCpuWidget()));
    connect(ui->actionSymbolInfo, SIGNAL(triggered()), this, SLOT(displaySymbolWidget()));
    connect(ui->actionModules, SIGNAL(triggered()), this, SLOT(displaySymbolWidget()));
    connect(ui->actionSource, SIGNAL(triggered()), this, SLOT(displaySourceViewWidget()));
    connect(mSymbolView, SIGNAL(showReferences()), this, SLOT(displayReferencesWidget()));
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
    connect(ui->actionCallStack, SIGNAL(triggered()), this, SLOT(displayCallstack()));
    connect(ui->actionSEHChain, SIGNAL(triggered()), this, SLOT(displaySEHChain()));
    connect(ui->actionTrace, SIGNAL(triggered()), this, SLOT(displayRunTrace()));
    connect(ui->actionDonate, SIGNAL(triggered()), this, SLOT(donate()));
    connect(ui->actionReportBug, SIGNAL(triggered()), this, SLOT(reportBug()));
    connect(ui->actionBlog, SIGNAL(triggered()), this, SLOT(blog()));
    connect(ui->actionCrashDump, SIGNAL(triggered()), this, SLOT(crashDump()));
    connect(ui->actionAttach, SIGNAL(triggered()), this, SLOT(displayAttach()));
    makeCommandAction(ui->actionDetach, "detach");
    connect(ui->actionChangeCommandLine, SIGNAL(triggered()), this, SLOT(changeCommandLine()));
    connect(ui->actionManual, SIGNAL(triggered()), this, SLOT(displayManual()));
    connect(ui->actionNotes, SIGNAL(triggered()), this, SLOT(displayNotesWidget()));
    connect(ui->actionSnowman, SIGNAL(triggered()), this, SLOT(displaySnowmanWidget()));
    connect(ui->actionHandles, SIGNAL(triggered()), this, SLOT(displayHandlesWidget()));
    connect(ui->actionGraph, SIGNAL(triggered()), this, SLOT(displayGraphWidget()));
    connect(ui->actionPreviousTab, SIGNAL(triggered()), this, SLOT(displayPreviousTab()));
    connect(ui->actionNextTab, SIGNAL(triggered()), this, SLOT(displayNextTab()));
    connect(ui->actionPreviousView, SIGNAL(triggered()), this, SLOT(displayPreviousView()));
    connect(ui->actionNextView, SIGNAL(triggered()), this, SLOT(displayNextView()));
    connect(ui->actionHideTab, SIGNAL(triggered()), this, SLOT(hideTab()));
    makeCommandAction(ui->actionStepIntoSource, "TraceIntoConditional src.line(cip) && !src.disp(cip)");
    makeCommandAction(ui->actionStepOverSource, "TraceOverConditional src.line(cip) && !src.disp(cip)");
    makeCommandAction(ui->actionseStepInto, "seStepInto");
    makeCommandAction(ui->actionseStepOver, "seStepOver");
    makeCommandAction(ui->actionseRun, "seRun");
    connect(ui->actionAnimateInto, SIGNAL(triggered()), this, SLOT(animateIntoSlot()));
    connect(ui->actionAnimateOver, SIGNAL(triggered()), this, SLOT(animateOverSlot()));
    connect(ui->actionAnimateCommand, SIGNAL(triggered()), this, SLOT(animateCommandSlot()));
    connect(ui->actionSetInitializationScript, SIGNAL(triggered()), this, SLOT(setInitializationScript()));
    connect(ui->actionCustomizeMenus, SIGNAL(triggered()), this, SLOT(customizeMenu()));
    connect(ui->actionVariables, SIGNAL(triggered()), this, SLOT(displayVariables()));

    connect(mCpuWidget->getDisasmWidget(), SIGNAL(updateWindowTitle(QString)), this, SLOT(updateWindowTitleSlot(QString)));
    connect(mCpuWidget->getDisasmWidget(), SIGNAL(displayReferencesWidget()), this, SLOT(displayReferencesWidget()));
    connect(mCpuWidget->getDisasmWidget(), SIGNAL(displaySourceManagerWidget()), this, SLOT(displaySourceViewWidget()));
    connect(mCpuWidget->getDisasmWidget(), SIGNAL(displaySnowmanWidget()), this, SLOT(displaySnowmanWidget()));
    connect(mCpuWidget->getDisasmWidget(), SIGNAL(displayLogWidget()), this, SLOT(displayLogWidget()));
    connect(mCpuWidget->getDisasmWidget(), SIGNAL(displayGraphWidget()), this, SLOT(displayGraphWidget()));
    connect(mCpuWidget->getDisasmWidget(), SIGNAL(displaySymbolsWidget()), this, SLOT(displaySymbolWidget()));
    connect(mCpuWidget->getDisasmWidget(), SIGNAL(showPatches()), this, SLOT(patchWindow()));

    connect(mGraphView, SIGNAL(displaySnowmanWidget()), this, SLOT(displaySnowmanWidget()));

    connect(mCpuWidget->getDumpWidget(), SIGNAL(displayReferencesWidget()), this, SLOT(displayReferencesWidget()));

    connect(mCpuWidget->getStackWidget(), SIGNAL(displayReferencesWidget()), this, SLOT(displayReferencesWidget()));

    connect(mTabWidget, SIGNAL(tabMovedTabWidget(int, int)), this, SLOT(tabMovedSlot(int, int)));
    connect(Config(), SIGNAL(shortcutsUpdated()), this, SLOT(refreshShortcuts()));

    // Setup favourite tools menu
    updateFavouriteTools();

    // Setup language menu
    setupLanguagesMenu();

    setupMenuCustomization();

    // Set default setttings (when not set)
    SettingsDialog defaultSettings;
    lastException = 0;
    defaultSettings.SaveSettings();
    // Don't need to set shortcuts because the code above will signal refreshShortcuts()

    mSimpleTraceDialog = new SimpleTraceDialog(this);

    // Update checker
    mUpdateChecker = new UpdateChecker(this);

    // Setup close thread and dialog
    bCanClose = false;
    mCloseThread = new MainWindowCloseThread(this);
    connect(mCloseThread, SIGNAL(canClose()), this, SLOT(canClose()));
    mCloseDialog = new CloseDialog(this);

    mCpuWidget->setDisasmFocus();

    QTimer::singleShot(0, this, SLOT(loadWindowSettings()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupCommandBar()
{
    mCmdLineEdit = new CommandLineEdit(ui->cmdBar);
    ui->cmdBar->addWidget(new QLabel(tr("Command: ")));
    ui->cmdBar->addWidget(mCmdLineEdit);
    ui->cmdBar->addWidget(mCmdLineEdit->selectorWidget());
}

void MainWindow::setupStatusBar()
{
    // Status label (Ready, Paused, ...)
    mStatusLabel = new DebugStatusLabel(ui->statusBar);
    mStatusLabel->setText(tr("Ready"));
    ui->statusBar->addWidget(mStatusLabel);

    // Log line
    mLastLogLabel = new LogStatusLabel(ui->statusBar);
    ui->statusBar->addPermanentWidget(mLastLogLabel, 1);

    // Time wasted counter
    QLabel* timeWastedLabel = new QLabel(this);
    ui->statusBar->addPermanentWidget(timeWastedLabel);
    mTimeWastedCounter = new TimeWastedCounter(this, timeWastedLabel);
}

void MainWindow::setupLanguagesMenu()
{
    QDir translationsDir(QString("%1/../translations/").arg(QCoreApplication::applicationDirPath()));
    QMenu* languageMenu;
    if(tr("Languages") == QString("Languages"))
        languageMenu = new QMenu(QString("Languages"));
    else
        languageMenu = new QMenu(tr("Languages") + QString(" Languages"), this);
    languageMenu->setIcon(DIcon("codepage.png"));
    QLocale enUS(QLocale::English, QLocale::UnitedStates);
    QString wCurrentLocale(currentLocale);
    QAction* action_enUS = new QAction(QString("[%1] %2 - %3").arg(enUS.name()).arg(enUS.nativeLanguageName()).arg(enUS.nativeCountryName()), languageMenu);
    connect(action_enUS, SIGNAL(triggered()), this, SLOT(chooseLanguage()));
    action_enUS->setCheckable(true);
    action_enUS->setChecked(false);
    languageMenu->addAction(action_enUS);
    if(!translationsDir.exists())
    {
        // translations dir do not exist
        action_enUS->setChecked(true);
        ui->menuOptions->addMenu(languageMenu);
        return;
    }
    if(wCurrentLocale == QString("en_US"))
        action_enUS->setChecked(true);
    QStringList filter;
    filter << "x64dbg_*.qm";
    QFileInfoList fileList = translationsDir.entryInfoList(filter, QDir::Readable | QDir::Files, QDir::Size);
    auto allLocales = QLocale::matchingLocales(QLocale::AnyLanguage, QLocale::AnyScript, QLocale::AnyCountry);
    for(auto i : fileList)
    {
        QString localeName = i.baseName().mid(7);
        for(auto j : allLocales)
        {
            if(j.name().startsWith(localeName))
            {
                QAction* actionLanguage = new QAction(QString("[%1] %2 - %3").arg(localeName).arg(j.nativeLanguageName()).arg(j.nativeCountryName()), languageMenu);
                connect(actionLanguage, SIGNAL(triggered()), this, SLOT(chooseLanguage()));
                actionLanguage->setCheckable(true);
                actionLanguage->setChecked(localeName == wCurrentLocale);
                languageMenu->addAction(actionLanguage);
                break;
            }
        }
    }
    ui->menuOptions->addMenu(languageMenu);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if(DbgIsDebugging() && ConfigBool("Gui", "ShowExitConfirmation"))
    {
        auto cb = new QCheckBox(tr("Don't ask this question again"));
        QMessageBox msgbox(this);
        msgbox.setText(tr("The debuggee is still running and will be terminated if you exit. Do you really want to exit?"));
        msgbox.setWindowTitle(tr("Debuggee is still running"));
        msgbox.setWindowIcon(DIcon("bug.png"));
        msgbox.addButton(QMessageBox::Yes);
        msgbox.addButton(QMessageBox::No);
        msgbox.setDefaultButton(QMessageBox::No);
        msgbox.setCheckBox(cb);

        QObject::connect(cb, &QCheckBox::toggled, [](bool checked)
        {
            Config()->setBool("Gui", "ShowExitConfirmation", !checked);
        });

        if(msgbox.exec() != QMessageBox::Yes)
        {
            event->ignore();
            return;
        }
    }

    duint noClose = 0;
    if(bCanClose)
    {
        saveWindowSettings();
    }
    if(BridgeSettingGetUint("Gui", "NoCloseDialog", &noClose) && noClose)
        mCloseDialog->hide();
    else
    {
        mCloseDialog->show();
        mCloseDialog->setFocus();
    }
    static volatile bool bExecuteThread = true;
    if(bExecuteThread)
    {
        bExecuteThread = false;
        CloseSnowman(mSnowmanView);
        Sleep(100);
        mCloseThread->start();
        emit Bridge::getBridge()->close();
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
    // shown tabs
    for(int i = 0; i < mTabWidget->count(); i++)
    {
        if(mTabWidget->widget(i) == widget)
        {
            mTabWidget->setCurrentIndex(i);
            return;
        }
    }

    // hidden tabs
    for(int i = 0; i < mWidgetList.count(); i++)
    {
        if(mWidgetList[i].widget == widget)
        {
            addQWidgetTab(mWidgetList[i].widget, mWidgetList[i].nativeName);
            mTabWidget->setCurrentIndex(mTabWidget->count() - 1);
            return;
        }
    }

    //TODO: restore configuration index
}

void MainWindow::loadTabDefaultOrder()
{
    clearTabWidget();

    // Setup tabs
    //TODO
    for(int i = 0; i < mWidgetList.size(); i++)
        addQWidgetTab(mWidgetList[i].widget, mWidgetList[i].nativeName);
}

void MainWindow::loadTabSavedOrder()
{
    clearTabWidget();

    QMap<duint, std::pair<QWidget*, QString>> tabIndexToWidget;

    // Get tabIndex for each widget and add them to tabIndexToWidget
    for(int i = 0; i < mWidgetList.size(); i++)
    {
        QString tabName = mWidgetList[i].nativeName;
        duint tabIndex = Config()->getUint("TabOrder", tabName);
        if(!tabIndexToWidget.contains(tabIndex))
            tabIndexToWidget.insert(tabIndex, std::make_pair(mWidgetList[i].widget, tabName));
        else
        {
            // Conflicts. Try to find an unused tab index.
            for(int j = 0; j < mWidgetList.size(); j++)
            {
                auto item = tabIndexToWidget.find(j);
                if(item == tabIndexToWidget.end())
                {
                    tabIndexToWidget.insert(j, std::make_pair(mWidgetList[i].widget, tabName));
                    break;
                }
            }
        }
    }

    // Setup tabs
    for(auto & widget : tabIndexToWidget)
        addQWidgetTab(widget.first, widget.second);
}

void MainWindow::clearTabWidget()
{
    if(mTabWidget->count() <= 0)
        return;

    // Remove all tabs starting from the end
    for(int i = mTabWidget->count() - 1; i >= 0; i--)
        mTabWidget->removeTab(i);
}

void MainWindow::saveWindowSettings()
{
    // Main Window settings
    BridgeSettingSet("Main Window Settings", "Geometry", saveGeometry().toBase64().data());
    BridgeSettingSet("Main Window Settings", "State", saveState().toBase64().data());

    // Set of currently detached tabs
    QSet<QWidget*> detachedTabWindows = mTabWidget->windows().toSet();

    // For all tabs, save detached status.  If detached, save geometry.
    for(int i = 0; i < mWidgetList.size(); i++)
    {
        bool isDetached = detachedTabWindows.contains(mWidgetList[i].widget);
        bool isDeleted = !isDetached && mTabWidget->indexOf(mWidgetList[i].widget) == -1;
        BridgeSettingSetUint("Detached Windows", mWidgetList[i].nativeName.toUtf8().constData(), isDetached);
        BridgeSettingSetUint("Deleted Tabs", mWidgetList[i].nativeName.toUtf8().constData(), isDeleted);
        if(isDetached)
            BridgeSettingSet("Tab Window Settings", mWidgetList[i].nativeName.toUtf8().constData(),
                             mWidgetList[i].widget->parentWidget()->saveGeometry().toBase64().data());
    }

    mCpuWidget->saveWindowSettings();
    mSymbolView->saveWindowSettings();
}

void MainWindow::loadWindowSettings()
{
    // Main Window settings
    char setting[MAX_SETTING_SIZE] = "";
    if(BridgeSettingGet("Main Window Settings", "Geometry", setting))
        restoreGeometry(QByteArray::fromBase64(QByteArray(setting)));

    if(BridgeSettingGet("Main Window Settings", "State", setting))
        restoreState(QByteArray::fromBase64(QByteArray(setting)));

    // Restore detached windows size and position
    // If a tab was detached last session, manually detach it now to populate MHTabWidget::windows
    for(int i = 0; i < mWidgetList.size(); i++)
    {
        duint isDetached = 0;
        BridgeSettingGetUint("Detached Windows", mWidgetList[i].nativeName.toUtf8().constData(), &isDetached);
        if(isDetached)
            mTabWidget->DetachTab(mTabWidget->indexOf(mWidgetList[i].widget), QPoint());
    }

    // Restore geometry for every tab we just detached
    QSet<QWidget*> detachedTabWindows = mTabWidget->windows().toSet();
    for(int i = 0; i < mWidgetList.size(); i++)
    {
        if(detachedTabWindows.contains(mWidgetList[i].widget))
        {
            if(BridgeSettingGet("Tab Window Settings", mWidgetList[i].nativeName.toUtf8().constData(), setting))
                mWidgetList[i].widget->parentWidget()->restoreGeometry(QByteArray::fromBase64(QByteArray(setting)));
        }
    }

    // 'Restore' deleted tabs
    for(int i = 0; i < mWidgetList.size(); i++)
    {
        duint isDeleted = 0;
        BridgeSettingGetUint("Deleted Tabs", mWidgetList[i].nativeName.toUtf8().constData(), &isDeleted);
        if(isDeleted)
            mTabWidget->DeleteTab(mTabWidget->indexOf(mWidgetList[i].widget));
    }

    mCpuWidget->loadWindowSettings();
    mSymbolView->loadWindowSettings();
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
    setGlobalShortcut(ui->actionImportdatabase, ConfigShortcut("FileImportDatabase"));
    setGlobalShortcut(ui->actionExportdatabase, ConfigShortcut("FileExportDatabase"));
    setGlobalShortcut(ui->actionRestartAdmin, ConfigShortcut("FileRestartAdmin"));
    setGlobalShortcut(ui->actionExit, ConfigShortcut("FileExit"));

    setGlobalShortcut(ui->actionCpu, ConfigShortcut("ViewCpu"));
    setGlobalShortcut(ui->actionLog, ConfigShortcut("ViewLog"));
    setGlobalShortcut(ui->actionBreakpoints, ConfigShortcut("ViewBreakpoints"));
    setGlobalShortcut(ui->actionMemoryMap, ConfigShortcut("ViewMemoryMap"));
    setGlobalShortcut(ui->actionCallStack, ConfigShortcut("ViewCallStack"));
    setGlobalShortcut(ui->actionSEHChain, ConfigShortcut("ViewSEHChain"));
    setGlobalShortcut(ui->actionScript, ConfigShortcut("ViewScript"));
    setGlobalShortcut(ui->actionSymbolInfo, ConfigShortcut("ViewSymbolInfo"));
    setGlobalShortcut(ui->actionModules, ConfigShortcut("ViewModules"));
    setGlobalShortcut(ui->actionSource, ConfigShortcut("ViewSource"));
    setGlobalShortcut(ui->actionReferences, ConfigShortcut("ViewReferences"));
    setGlobalShortcut(ui->actionThreads, ConfigShortcut("ViewThreads"));
    setGlobalShortcut(ui->actionPatches, ConfigShortcut("ViewPatches"));
    setGlobalShortcut(ui->actionComments, ConfigShortcut("ViewComments"));
    setGlobalShortcut(ui->actionLabels, ConfigShortcut("ViewLabels"));
    setGlobalShortcut(ui->actionBookmarks, ConfigShortcut("ViewBookmarks"));
    setGlobalShortcut(ui->actionFunctions, ConfigShortcut("ViewFunctions"));
    setGlobalShortcut(ui->actionVariables, ConfigShortcut("ViewVariables"));
    setGlobalShortcut(ui->actionSnowman, ConfigShortcut("ViewSnowman"));
    setGlobalShortcut(ui->actionHandles, ConfigShortcut("ViewHandles"));
    setGlobalShortcut(ui->actionGraph, ConfigShortcut("ViewGraph"));
    setGlobalShortcut(ui->actionPreviousTab, ConfigShortcut("ViewPreviousTab"));
    setGlobalShortcut(ui->actionNextTab, ConfigShortcut("ViewNextTab"));
    setGlobalShortcut(ui->actionPreviousView, ConfigShortcut("ViewPreviousHistory"));
    setGlobalShortcut(ui->actionNextView, ConfigShortcut("ViewNextHistory"));
    setGlobalShortcut(ui->actionHideTab, ConfigShortcut("ViewHideTab"));

    setGlobalShortcut(ui->actionRun, ConfigShortcut("DebugRun"));
    setGlobalShortcut(ui->actioneRun, ConfigShortcut("DebugeRun"));
    setGlobalShortcut(ui->actionseRun, ConfigShortcut("DebugseRun"));
    setGlobalShortcut(ui->actionRunSelection, ConfigShortcut("DebugRunSelection"));
    setGlobalShortcut(ui->actionRunExpression, ConfigShortcut("DebugRunExpression"));
    setGlobalShortcut(ui->actionPause, ConfigShortcut("DebugPause"));
    setGlobalShortcut(ui->actionRestart, ConfigShortcut("DebugRestart"));
    setGlobalShortcut(ui->actionClose, ConfigShortcut("DebugClose"));
    setGlobalShortcut(ui->actionStepInto, ConfigShortcut("DebugStepInto"));
    setGlobalShortcut(ui->actioneStepInto, ConfigShortcut("DebugeStepInto"));
    setGlobalShortcut(ui->actionseStepInto, ConfigShortcut("DebugseStepInto"));
    setGlobalShortcut(ui->actionStepIntoSource, ConfigShortcut("DebugStepIntoSource"));
    setGlobalShortcut(ui->actionStepOver, ConfigShortcut("DebugStepOver"));
    setGlobalShortcut(ui->actioneStepOver, ConfigShortcut("DebugeStepOver"));
    setGlobalShortcut(ui->actionseStepOver, ConfigShortcut("DebugseStepOver"));
    setGlobalShortcut(ui->actionStepOverSource, ConfigShortcut("DebugStepOverSource"));
    setGlobalShortcut(ui->actionRtr, ConfigShortcut("DebugRtr"));
    setGlobalShortcut(ui->actioneRtr, ConfigShortcut("DebugeRtr"));
    setGlobalShortcut(ui->actionRtu, ConfigShortcut("DebugRtu"));
    setGlobalShortcut(ui->actionCommand, ConfigShortcut("DebugCommand"));
    setGlobalShortcut(ui->actionSkipNextInstruction, ConfigShortcut("DebugSkipNextInstruction"));
    setGlobalShortcut(ui->actionTicnd, ConfigShortcut("DebugTraceIntoConditional"));
    setGlobalShortcut(ui->actionTocnd, ConfigShortcut("DebugTraceOverConditional"));
    setGlobalShortcut(ui->actionTRBit, ConfigShortcut("DebugEnableTraceRecordBit"));
    setGlobalShortcut(ui->actionTRNone, ConfigShortcut("DebugTraceRecordNone"));
    setGlobalShortcut(ui->actionInstrUndo, ConfigShortcut("DebugInstrUndo"));
    setGlobalShortcut(ui->actionAnimateInto, ConfigShortcut("DebugAnimateInto"));
    setGlobalShortcut(ui->actionAnimateOver, ConfigShortcut("DebugAnimateOver"));
    setGlobalShortcut(ui->actionAnimateCommand, ConfigShortcut("DebugAnimateCommand"));
    setGlobalShortcut(ui->actionTRTIIT, ConfigShortcut("DebugTraceIntoIntoTracerecord"));
    setGlobalShortcut(ui->actionTRTOIT, ConfigShortcut("DebugTraceOverIntoTracerecord"));
    setGlobalShortcut(ui->actionTRTIBT, ConfigShortcut("DebugTraceIntoBeyondTracerecord"));
    setGlobalShortcut(ui->actionTRTOBT, ConfigShortcut("DebugTraceOverBeyondTracerecord"));

    setGlobalShortcut(ui->actionScylla, ConfigShortcut("PluginsScylla"));

    setGlobalShortcut(actionManageFavourites, ConfigShortcut("FavouritesManage"));

    setGlobalShortcut(ui->actionSettings, ConfigShortcut("OptionsPreferences"));
    setGlobalShortcut(ui->actionAppearance, ConfigShortcut("OptionsAppearance"));
    setGlobalShortcut(ui->actionShortcuts, ConfigShortcut("OptionsShortcuts"));
    setGlobalShortcut(ui->actionTopmost, ConfigShortcut("OptionsTopmost"));
    setGlobalShortcut(ui->actionReloadStylesheet, ConfigShortcut("OptionsReloadStylesheet"));

    setGlobalShortcut(ui->actionAbout, ConfigShortcut("HelpAbout"));
    setGlobalShortcut(ui->actionBlog, ConfigShortcut("HelpBlog"));
    setGlobalShortcut(ui->actionDonate, ConfigShortcut("HelpDonate"));
    setGlobalShortcut(ui->actionCalculator, ConfigShortcut("HelpCalculator"));
    setGlobalShortcut(ui->actionReportBug, ConfigShortcut("HelpReportBug"));
    setGlobalShortcut(ui->actionManual, ConfigShortcut("HelpManual"));
    setGlobalShortcut(ui->actionCrashDump, ConfigShortcut("HelpCrashDump"));

    setGlobalShortcut(ui->actionStrings, ConfigShortcut("ActionFindStrings"));
    setGlobalShortcut(ui->actionCalls, ConfigShortcut("ActionFindIntermodularCalls"));

    for(const MenuEntryInfo & entry : mEntryList)
        if(!entry.hotkeyId.isEmpty())
            entry.mAction->setShortcut(ConfigShortcut(entry.hotkeyId));
}

void MainWindow::updateMRUMenu()
{
    QMenu* fileMenu = ui->menuRecentFiles;
    QList<QAction*> list = fileMenu->actions();
    for(int i = 1; i < list.length(); ++i)
        fileMenu->removeAction(list.at(i));
    mMRUList->appendMenu(fileMenu);
}

void MainWindow::executeCommand()
{
    mCmdLineEdit->execute();
}

QAction* MainWindow::makeCommandAction(QAction* action, const QString & command)
{
    action->setData(QVariant(command));
    connect(action, SIGNAL(triggered()), this, SLOT(execCommandSlot()));
    return action;
}

void MainWindow::execCommandSlot()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if(action)
        DbgCmdExec(action->data().toString().toUtf8().constData());
}

void MainWindow::setFocusToCommandBar()
{
    mCmdLineEdit->setFocus();
}

void MainWindow::execTRBit()
{
    mCpuWidget->getDisasmWidget()->ActionTraceRecordBitSlot();
}

void MainWindow::execTRByte()
{
    mCpuWidget->getDisasmWidget()->ActionTraceRecordByteSlot();
}

void MainWindow::execTRWord()
{
    mCpuWidget->getDisasmWidget()->ActionTraceRecordWordSlot();
}

void MainWindow::execTRNone()
{
    mCpuWidget->getDisasmWidget()->ActionTraceRecordDisableSlot();
}

void MainWindow::execTicnd()
{
    if(!DbgIsDebugging())
        return;
    mSimpleTraceDialog->setTraceCommand("TraceIntoConditional");
    mSimpleTraceDialog->setWindowTitle(tr("Trace into..."));
    mSimpleTraceDialog->setWindowIcon(DIcon("traceinto.png"));
    mSimpleTraceDialog->exec();
}

void MainWindow::execTocnd()
{
    if(!DbgIsDebugging())
        return;
    mSimpleTraceDialog->setTraceCommand("TraceOverConditional");
    mSimpleTraceDialog->setWindowTitle(tr("Trace over..."));
    mSimpleTraceDialog->setWindowIcon(DIcon("traceover.png"));
    mSimpleTraceDialog->exec();
}

void MainWindow::displayMemMapWidget()
{
    showQWidgetTab(mMemMapView);
}

void MainWindow::displayVariables()
{
    DbgCmdExec("varlist");
    showQWidgetTab(mReferenceManager);
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
    AboutDialog dialog(mUpdateChecker, this);
    dialog.exec();
}

void MainWindow::openFileSlot()
{
    auto filename = QFileDialog::getOpenFileName(this, tr("Open file"), mMRUList->getEntry(0), tr("Executables (*.exe *.dll);;All files (*.*)"));
    if(!filename.length())
        return;
    filename = QDir::toNativeSeparators(filename); //convert to native path format (with backlashes)
    openRecentFileSlot(filename);
}

void MainWindow::openRecentFileSlot(QString filename)
{
    DbgCmdExec(QString().sprintf("init \"%s\"", filename.toUtf8().constData()).toUtf8().constData());
}

void MainWindow::runSlot()
{
    if(DbgIsDebugging())
        DbgCmdExec("run");
    else
        restartDebugging();
}

void MainWindow::restartDebugging()
{
    auto last = mMRUList->getEntry(0);
    if(!last.isEmpty())
        DbgCmdExec(QString("init \"%1\"").arg(last).toUtf8().constData());
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

bool MainWindow::event(QEvent* event)
{
    // just make sure mTabWidget take current view as the latest
    if(event->type() == QEvent::WindowActivate && this->isActiveWindow())
    {
        mTabWidget->setCurrentIndex(mTabWidget->currentIndex());
    }

    return QMainWindow::event(event);
}

void MainWindow::updateWindowTitleSlot(QString filename)
{
    if(filename.length())
    {
        setWindowTitle(mWindowMainTitle + QString(" - ") + filename);
        windowTitle = filename;
    }
    else
    {
        setWindowTitle(mWindowMainTitle);
        windowTitle = mWindowMainTitle;
    }
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

void MainWindow::displayGraphWidget()
{
    showQWidgetTab(mGraphView);
}

void MainWindow::displayPreviousTab()
{
    mTabWidget->showPreviousTab();
}

void MainWindow::displayNextTab()
{
    mTabWidget->showNextTab();
}

void MainWindow::displayPreviousView()
{
    mTabWidget->showPreviousView();
}

void MainWindow::displayNextView()
{
    mTabWidget->showNextView();
}

void MainWindow::hideTab()
{
    mTabWidget->deleteCurrentTab();
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
    mMRUList->addEntry(file);
    updateMRUMenu();
    mMRUList->save();
}

void MainWindow::setLastException(unsigned int exceptionCode)
{
    lastException = exceptionCode;
}

void MainWindow::findStrings()
{
    DbgCmdExec(QString("strref " + ToPtrString(mCpuWidget->getDisasmWidget()->getSelectedVa())).toUtf8().constData());
    displayReferencesWidget();
}

void MainWindow::findModularCalls()
{
    DbgCmdExec(QString("modcallfind " + ToPtrString(mCpuWidget->getDisasmWidget()->getSelectedVa())).toUtf8().constData());
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
        mMenuList.push_back(MenuInfo(parent, menu, hMenu, hParentMenu, hMenu == GUI_PLUGIN_MENU));
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
    int hMenuNew = hEntryMenuPool++;
    QWidget* parent = hMenu == -1 ? this : menu->parent;
    QMenu* wMenu = new QMenu(title, parent);
    wMenu->menuAction()->setVisible(false);
    mMenuList.push_back(MenuInfo(parent, wMenu, hMenuNew, hMenu, !menu || menu->globalMenu));
    if(hMenu == -1) //top-level
        ui->menuBar->addMenu(wMenu);
    else //deeper level
    {
        menu->mMenu->addMenu(wMenu);
        menu->mMenu->menuAction()->setVisible(true);
    }
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
    int hEntryNew = hEntryMenuPool++;
    newInfo.hEntry = hEntryNew;
    newInfo.hParentMenu = hMenu;
    QWidget* parent = hMenu == -1 ? this : menu->parent;
    QAction* wAction = new QAction(title, parent);
    wAction->setObjectName(QString().sprintf("ENTRY|%d", hEntryNew));
    wAction->setShortcutContext((!menu || menu->globalMenu) ? Qt::ApplicationShortcut : Qt::WidgetShortcut);
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

void MainWindow::clearMenuHelper(int hMenu)
{
    //delete menu entries
    for(auto i = mEntryList.size() - 1; i != -1; i--)
        if(hMenu == mEntryList.at(i).hParentMenu) //we found an entry that has the menu as parent
            mEntryList.erase(mEntryList.begin() + i);
    //delete the menus
    std::vector<int> menuClearQueue;
    for(auto i = mMenuList.size() - 1; i != -1; i--)
    {
        if(hMenu == mMenuList.at(i).hParentMenu) //we found a menu that has the menu as parent
        {
            menuClearQueue.push_back(mMenuList.at(i).hMenu);
            mMenuList.erase(mMenuList.begin() + i);
        }
    }
    //recursively clear the menus
    for(auto & hMenu : menuClearQueue)
        clearMenuHelper(hMenu);
}

void MainWindow::clearMenu(int hMenu, bool erase)
{
    //this recursively removes the entries from mEntryList and mMenuList
    clearMenuHelper(hMenu);
    for(auto it = mMenuList.begin(); it != mMenuList.end(); ++it)
    {
        auto & curMenu = *it;
        if(hMenu == curMenu.hMenu)
        {
            if(erase)
            {
                auto parentMenu = findMenu(curMenu.hParentMenu);
                if(parentMenu)
                {
                    parentMenu->mMenu->removeAction(curMenu.mMenu->menuAction()); //remove the QMenu from the parent
                    if(parentMenu->mMenu->actions().empty()) //hide the parent if it is now empty
                        parentMenu->mMenu->menuAction()->setVisible(false);
                }
                it = mMenuList.erase(it);
            }
            else
            {
                curMenu.mMenu->clear(); //clear the QMenu
                curMenu.mMenu->menuAction()->setVisible(false);
            }
            break;
        }
    }
    Bridge::getBridge()->setResult();
}

void MainWindow::initMenuApi()
{
    //256 entries are reserved
    hEntryMenuPool = 256;
    mEntryList.reserve(1024);
    mMenuList.reserve(1024);
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

void MainWindow::removeMenuEntry(int hEntryMenu)
{
    //find and remove the hEntryMenu from the mEntryList
    for(int i = 0; i < mEntryList.size(); i++)
    {
        if(mEntryList.at(i).hEntry == hEntryMenu)
        {
            auto & entry = mEntryList.at(i);
            auto parentMenu = findMenu(entry.hParentMenu);
            if(parentMenu)
            {
                parentMenu->mMenu->removeAction(entry.mAction);
                if(parentMenu->mMenu->actions().empty())
                    parentMenu->mMenu->menuAction()->setVisible(false);
                mEntryList.erase(mEntryList.begin() + i);
            }
            Bridge::getBridge()->setResult();
            return;
        }
    }
    //if hEntryMenu is not in mEntryList, clear+erase it from mMenuList
    clearMenu(hEntryMenu, true);
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

void MainWindow::setCheckedMenuEntry(int hEntry, bool checked)
{
    for(int i = 0; i < mEntryList.size(); i++)
    {
        if(mEntryList.at(i).hEntry == hEntry)
        {
            const MenuEntryInfo & entry = mEntryList.at(i);
            entry.mAction->setCheckable(true);
            entry.mAction->setChecked(checked);
            break;
        }
    }
    Bridge::getBridge()->setResult();
}

QString MainWindow::nestedMenuDescription(const MenuInfo* menu)
{
    auto found = findMenu(menu->hParentMenu);
    if(!found)
        return menu->mMenu->title();
    auto nest = nestedMenuDescription(found);
    if(nest.isEmpty())
    {
        switch(menu->hParentMenu)
        {
        case GUI_DISASM_MENU:
            nest = tr("&Plugins") + " -> " + tr("Disassembly");
            break;
        case GUI_DUMP_MENU:
            nest = tr("&Plugins") + " -> " + tr("Dump");
            break;
        case GUI_STACK_MENU:
            nest = tr("&Plugins") + " -> " + tr("Stack");
            break;
        }
    }
    nest += " -> ";
    return nest + menu->mMenu->title();
}

QString MainWindow::nestedMenuEntryDescription(const MenuEntryInfo & entry)
{
    return QString(nestedMenuDescription(findMenu(entry.hParentMenu)) + " -> " + entry.mAction->text()).replace("&", "");
}

void MainWindow::setHotkeyMenuEntry(int hEntry, QString hotkey, QString id)
{
    for(int i = 0; i < mEntryList.size(); i++)
    {
        if(mEntryList.at(i).hEntry == hEntry)
        {
            MenuEntryInfo & entry = mEntryList[i];
            entry.hotkeyId = QString("Plugin_") + id;
            id.truncate(id.lastIndexOf('_'));
            entry.hotkey = hotkey;
            entry.hotkeyGlobal = entry.mAction->shortcutContext() == Qt::ApplicationShortcut;
            Config()->setPluginShortcut(entry.hotkeyId, nestedMenuEntryDescription(entry), hotkey, entry.hotkeyGlobal);
            refreshShortcuts();
            break;
        }
    }
    Bridge::getBridge()->setResult();
}

void MainWindow::setVisibleMenuEntry(int hEntry, bool visible)
{
    for(int i = 0; i < mEntryList.size(); i++)
    {
        if(mEntryList.at(i).hEntry == hEntry)
        {
            const MenuEntryInfo & entry = mEntryList.at(i);
            entry.mAction->setVisible(visible);
            break;
        }
    }
    Bridge::getBridge()->setResult();
}

void MainWindow::setVisibleMenu(int hMenu, bool visible)
{
    for(int i = 0; i < mMenuList.size(); i++)
    {
        if(mMenuList.at(i).hMenu == hMenu)
        {
            const MenuInfo & menu = mMenuList.at(i);
            menu.mMenu->setVisible(visible);
        }
    }
    Bridge::getBridge()->setResult();
}

void MainWindow::setNameMenuEntry(int hEntry, QString name)
{
    for(int i = 0; i < mEntryList.size(); i++)
    {
        if(mEntryList.at(i).hEntry == hEntry)
        {
            const MenuEntryInfo & entry = mEntryList.at(i);
            entry.mAction->setText(name);
            Config()->setPluginShortcut(entry.hotkeyId, nestedMenuEntryDescription(entry), entry.hotkey, entry.hotkeyGlobal);
            break;
        }
    }
    Bridge::getBridge()->setResult();
}

void MainWindow::setNameMenu(int hMenu, QString name)
{
    for(int i = 0; i < mMenuList.size(); i++)
    {
        if(mMenuList.at(i).hMenu == hMenu)
        {
            const MenuInfo & menu = mMenuList.at(i);
            menu.mMenu->setTitle(name);
        }
    }
    Bridge::getBridge()->setResult();
}

void MainWindow::runSelection()
{
    if(DbgIsDebugging())
        DbgCmdExec(("run " + ToPtrString(mGraphView->hasFocus() ? mGraphView->get_cursor_pos() : mCpuWidget->getDisasmWidget()->getSelectedVa())).toUtf8().constData());
}

void MainWindow::runExpression()
{
    if(!DbgIsDebugging())
        return;

    GotoDialog gotoDialog(this);
    gotoDialog.setWindowTitle(tr("Enter expression to run to..."));
    if(gotoDialog.exec() != QDialog::Accepted)
        return;

    if(DbgCmdExecDirect(QString("bp \"%1\", ss").arg(gotoDialog.expressionText).toUtf8().constData()))
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
        SimpleErrorBox(this, tr("Error!"), tr("Patches cannot be shown when not debugging..."));
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

void MainWindow::displayCallstack()
{
    showQWidgetTab(mCallStackView);
}

void MainWindow::displaySEHChain()
{
    showQWidgetTab(mSEHChainView);
}

void MainWindow::displayRunTrace()
{
    showQWidgetTab(mTraceBrowser);
}

void MainWindow::donate()
{
    QMessageBox msg(QMessageBox::Information, tr("Donate"), tr("All the money will go to x64dbg development."));
    msg.setWindowIcon(DIcon("donate.png"));
    msg.setParent(this, Qt::Dialog);
    msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
    msg.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msg.setDefaultButton(QMessageBox::Ok);
    if(msg.exec() != QMessageBox::Ok)
        return;
    QDesktopServices::openUrl(QUrl("http://donate.x64dbg.com"));
}

void MainWindow::blog()
{
    QMessageBox msg(QMessageBox::Information, tr("Blog"), tr("You will visit x64dbg's official blog."));
    msg.setWindowIcon(DIcon("hex.png"));
    msg.setParent(this, Qt::Dialog);
    msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
    msg.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msg.setDefaultButton(QMessageBox::Ok);
    if(msg.exec() != QMessageBox::Ok)
        return;
    QDesktopServices::openUrl(QUrl("http://blog.x64dbg.com"));
}

void MainWindow::reportBug()
{
    QMessageBox msg(QMessageBox::Information, tr("Report Bug"), tr("You will be taken to a website where you can report a bug.\nMake sure to fill in as much information as possible."));
    msg.setWindowIcon(DIcon("bug-report.png"));
    msg.setParent(this, Qt::Dialog);
    msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
    msg.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msg.setDefaultButton(QMessageBox::Ok);
    if(msg.exec() != QMessageBox::Ok)
        return;
    QDesktopServices::openUrl(QUrl("http://report.x64dbg.com"));
}

void MainWindow::crashDump()
{
    QMessageBox msg(QMessageBox::Critical, tr("Generate crash dump"), tr("This action will crash the debugger and generate a crash dump. You will LOSE ALL YOUR UNSAVED DATA. Do you really want to continue?"));
    msg.setWindowIcon(DIcon("fatal-error.png"));
    msg.setParent(this, Qt::Dialog);
    msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
    msg.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msg.setDefaultButton(QMessageBox::Cancel);
    if(msg.exec() != QMessageBox::Ok)
        return;

    // Fatal error
    __debugbreak();

    // Congratulations! We survived a fatal error!
    SimpleWarningBox(this, tr("Have fun debugging the debugger!"), tr("Debugger detected!"));
}

void MainWindow::displayAttach()
{
    AttachDialog attach(this);
    attach.exec();

    mCpuWidget->setDisasmFocus();
}

static bool getCmdLine(QString & cmdLine)
{
    auto result = false;
    size_t cbsize = 0;
    if(DbgFunctions()->GetCmdline(0, &cbsize))
    {
        auto buffer = new char[cbsize];
        if(DbgFunctions()->GetCmdline(buffer, 0))
        {
            cmdLine = buffer;
            result = true;
        }
        delete[] buffer;
    }
    return result;
}

void MainWindow::changeCommandLine()
{
    if(!DbgIsDebugging())
        return;

    LineEditDialog mLineEdit(this);
    mLineEdit.setText("");
    mLineEdit.setWindowTitle(tr("Change Command Line"));
    mLineEdit.setWindowIcon(DIcon("changeargs.png"));

    QString cmdLine;
    if(!getCmdLine(cmdLine))
        mLineEdit.setText(tr("Cannot get remote command line, use the 'getcmdline' command for more information."));
    else
        mLineEdit.setText(cmdLine);

    mLineEdit.setCursorPosition(0);

    if(mLineEdit.exec() != QDialog::Accepted)
        return; //pressed cancel

    if(!DbgFunctions()->SetCmdline((char*)mLineEdit.editText.toUtf8().constData()))
        SimpleErrorBox(this, tr("Error!"), tr("Could not set command line!"));
    else
    {
        DbgFunctions()->MemUpdateMap();
        GuiUpdateMemoryView();
        getCmdLine(cmdLine);
        GuiAddLogMessage((tr("New command line: ") + cmdLine + "\n").toUtf8().constData());
    }
}

void MainWindow::displayManual()
{
    duint setting = 0;
    if(BridgeSettingGetUint("Misc", "UseLocalHelpFile", &setting) && setting)
    {
        // Open the Windows CHM in the upper directory
        if(!QDesktopServices::openUrl(QUrl(QUrl::fromLocalFile(QString("%1/../x64dbg.chm").arg(QCoreApplication::applicationDirPath())))))
            SimpleErrorBox(this, tr("Error"), tr("Manual cannot be opened. Please check if x64dbg.chm exists and ensure there is no other problems with your system."));
    }
    else
        QDesktopServices::openUrl(QUrl("http://help.x64dbg.com"));
}

void MainWindow::canClose()
{
    bCanClose = true;
    close();
}

void MainWindow::addQWidgetTab(QWidget* qWidget, QString nativeName)
{
    mTabWidget->addTabEx(qWidget, qWidget->windowIcon(), qWidget->windowTitle(), nativeName);
}

void MainWindow::addQWidgetTab(QWidget* qWidget)
{
    addQWidgetTab(qWidget, qWidget->windowTitle());
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
        //QString tabName = mTabWidget->tabText(i).replace(" ", "") + "Tab";
        QString tabName = mTabWidget->getNativeName(i);
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

void MainWindow::dbgStateChangedSlot(DBGSTATE state)
{
    if(state == initialized) //fixes a crash when restarting with certain settings in another tab
        displayCpuWidget();
}

void MainWindow::on_actionFaq_triggered()
{
    QDesktopServices::openUrl(QUrl("http://faq.x64dbg.com"));
}

void MainWindow::on_actionReloadStylesheet_triggered()
{
    QFile f(QString("%1/style.css").arg(QCoreApplication::applicationDirPath()));
    if(f.open(QFile::ReadOnly | QFile::Text))
    {
        QTextStream in(&f);
        auto style = in.readAll();
        f.close();
        qApp->setStyleSheet(style);
    }
    else
        qApp->setStyleSheet("");
    ensurePolished();
    update();
}

void MainWindow::displayNotesWidget()
{
    showQWidgetTab(mNotesManager);
}

void MainWindow::displayHandlesWidget()
{
    showQWidgetTab(mHandlesView);
}

void MainWindow::manageFavourites()
{
    FavouriteTools favToolsDialog(this);
    favToolsDialog.exec();
    updateFavouriteTools();
}

void MainWindow::updateFavouriteTools()
{
    char buffer[MAX_SETTING_SIZE];
    bool isanythingexists = false;
    ui->menuFavourites->clear();
    for(unsigned int i = 1; BridgeSettingGet("Favourite", QString("Tool%1").arg(i).toUtf8().constData(), buffer); i++)
    {
        QString exePath = QString(buffer);
        QAction* newAction = new QAction(this);
        newAction->setData(QVariant(QString("Tool,%1").arg(exePath)));
        if(BridgeSettingGet("Favourite", QString("ToolShortcut%1").arg(i).toUtf8().constData(), buffer))
            if(*buffer && strcmp(buffer, "NOT_SET") != 0)
                setGlobalShortcut(newAction, QKeySequence(QString(buffer)));
        if(BridgeSettingGet("Favourite", QString("ToolDescription%1").arg(i).toUtf8().constData(), buffer))
            newAction->setText(QString(buffer));
        else
            newAction->setText(exePath);
        connect(newAction, SIGNAL(triggered()), this, SLOT(clickFavouriteTool()));
        ui->menuFavourites->addAction(newAction);
        isanythingexists = true;
    }
    if(isanythingexists)
    {
        isanythingexists = false;
        ui->menuFavourites->addSeparator();
    }
    for(unsigned int i = 1; BridgeSettingGet("Favourite", QString("Script%1").arg(i).toUtf8().constData(), buffer); i++)
    {
        QString scriptPath = QString(buffer);
        QAction* newAction = new QAction(this);
        newAction->setData(QVariant(QString("Script,%1").arg(scriptPath)));
        if(BridgeSettingGet("Favourite", QString("ScriptShortcut%1").arg(i).toUtf8().constData(), buffer))
            if(*buffer && strcmp(buffer, "NOT_SET") != 0)
                setGlobalShortcut(newAction, QKeySequence(QString(buffer)));
        if(BridgeSettingGet("Favourite", QString("ScriptDescription%1").arg(i).toUtf8().constData(), buffer))
            newAction->setText(QString(buffer));
        else
            newAction->setText(scriptPath);
        connect(newAction, SIGNAL(triggered()), this, SLOT(clickFavouriteTool()));
        ui->menuFavourites->addAction(newAction);
        isanythingexists = true;
    }
    if(isanythingexists)
    {
        isanythingexists = false;
        ui->menuFavourites->addSeparator();
    }
    for(unsigned int i = 1; BridgeSettingGet("Favourite", QString("Command%1").arg(i).toUtf8().constData(), buffer); i++)
    {
        QAction* newAction = new QAction(QString(buffer), this);
        newAction->setData(QVariant(QString("Command")));
        if(BridgeSettingGet("Favourite", QString("CommandShortcut%1").arg(i).toUtf8().constData(), buffer))
            if(*buffer && strcmp(buffer, "NOT_SET") != 0)
                setGlobalShortcut(newAction, QKeySequence(QString(buffer)));
        connect(newAction, SIGNAL(triggered()), this, SLOT(clickFavouriteTool()));
        ui->menuFavourites->addAction(newAction);
        isanythingexists = true;
    }
    if(isanythingexists)
        ui->menuFavourites->addSeparator();
    actionManageFavourites = new QAction(DIcon("star.png"), tr("&Manage Favourite Tools..."), this);
    ui->menuFavourites->addAction(actionManageFavourites);
    setGlobalShortcut(actionManageFavourites, ConfigShortcut("FavouritesManage"));
    connect(ui->menuFavourites->actions().last(), SIGNAL(triggered()), this, SLOT(manageFavourites()));
}

static QString stringFormatInline(const QString & format)
{
    if(!DbgFunctions()->StringFormatInline)
        return QString();
    char result[MAX_SETTING_SIZE] = "";
    if(DbgFunctions()->StringFormatInline(format.toUtf8().constData(), MAX_SETTING_SIZE, result))
        return result;
    return CPUArgumentWidget::tr("[Formatting Error]");
}

void MainWindow::clickFavouriteTool()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if(action == nullptr)
        return;
    QString data = action->data().toString();
    if(data.startsWith("Tool,"))
    {
        QString toolPath = data.mid(5);
        duint PID = DbgValFromString("$pid");
        toolPath.replace(QString("%PID%"), QString::number(PID), Qt::CaseInsensitive);
        toolPath.replace(QString("%DEBUGGEE%"), mMRUList->getEntry(0), Qt::CaseInsensitive);
        char modpath[MAX_MODULE_SIZE] = "";
        DbgFunctions()->ModPathFromAddr(DbgValFromString("dis.sel()"), modpath, MAX_MODULE_SIZE);
        toolPath.replace(QString("%MODULE%"), modpath, Qt::CaseInsensitive);
        while(true)
        {
            auto sfStart = toolPath.indexOf("%-");
            auto sfEnd = toolPath.indexOf("-%");
            if(sfStart < 0 || sfEnd < 0 || sfEnd < sfStart)
                break;
            auto format = toolPath.mid(sfStart + 2, sfEnd - sfStart - 2);
            toolPath.replace(sfStart, sfEnd - sfStart + 2, stringFormatInline(format));
        }
        mLastLogLabel->setText(toolPath);
        PROCESS_INFORMATION procinfo;
        STARTUPINFO startupinfo;
        memset(&procinfo, 0, sizeof(PROCESS_INFORMATION));
        memset(&startupinfo, 0, sizeof(startupinfo));
        startupinfo.cb = sizeof(startupinfo);
        if(CreateProcessW(nullptr, (LPWSTR)toolPath.toStdWString().c_str(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startupinfo, &procinfo))
        {
            CloseHandle(procinfo.hThread);
            CloseHandle(procinfo.hProcess);
        }
        else if(GetLastError() == ERROR_ELEVATION_REQUIRED)
        {
            QString file, cmd;
            if(toolPath.startsWith('\"'))
            {
                auto endQuote = toolPath.indexOf('\"', 1);
                if(endQuote == -1) //"failure with spaces
                    file = toolPath.mid(1);
                else //"path with spaces" arguments
                {
                    file = toolPath.mid(1, endQuote - 1);
                    cmd = toolPath.mid(endQuote + 1);
                }
            }
            else
            {
                auto firstSpace = toolPath.indexOf(' ');
                if(firstSpace == -1) //pathwithoutspaces
                    file = toolPath;
                else //pathwithoutspaces argument
                {
                    file = toolPath.left(firstSpace);
                    cmd = toolPath.mid(firstSpace + 1);
                }
            }
            file = file.trimmed();
            cmd = cmd.trimmed();
            ShellExecuteW(nullptr, L"runas", file.toStdWString().c_str(), cmd.toStdWString().c_str(), nullptr, SW_SHOWNORMAL);
        }
    }
    else if(data.startsWith("Script,"))
    {
        QString scriptPath = data.mid(7);
        mScriptView->openRecentFile(scriptPath);
        displayScriptWidget();
    }
    else if(data.compare("Command") == 0)
    {
        DbgCmdExec(action->text().toUtf8().constData());
    }
}

void MainWindow::chooseLanguage()
{
    QAction* action = qobject_cast<QAction*>(sender());
    QString localeName = action->text();
    localeName = localeName.mid(1, localeName.indexOf(QChar(']')) - 1);
    action->setChecked(localeName == QString(currentLocale));
    if(localeName != "en_US")
    {
        QDir translationsDir(QString("%1/../translations/").arg(QCoreApplication::applicationDirPath()));
        QFile file(translationsDir.absoluteFilePath(QString("x64dbg_%1.qm").arg(localeName)));
        if(file.size() < 512)
        {
            QMessageBox msg(this);
            msg.setWindowIcon(DIcon("codepage.png"));
            msg.setIcon(QMessageBox::Information);
            if(tr("Languages") == QString("Languages"))
            {
                msg.setWindowTitle(QString("Languages"));
                msg.setText(QString("The translation is nearly empty. Do you still want to use this language?"));
            }
            else
            {
                msg.setWindowTitle(tr("Languages") + QString(" Languages"));
                msg.setText(tr("The translation is nearly empty. Do you still want to use this language?") + QString("\r\nThe translation is nearly empty. Do you still want to use this language?"));
            }
            msg.addButton(QMessageBox::Yes);
            msg.addButton(QMessageBox::No);
            if(msg.exec() == QMessageBox::No)
                return;
        }
    }
    BridgeSettingSet("Engine", "Language", localeName.toUtf8().constData());
    QMessageBox msg(this);
    msg.setIcon(QMessageBox::Information);
    msg.setWindowIcon(DIcon("codepage.png"));
    if(tr("Languages") == QString("Languages"))
    {
        msg.setWindowTitle(QString("Languages"));
        msg.setText(QString("New language setting will take effect upon restart."));
    }
    else
    {
        msg.setWindowTitle(tr("Languages") + QString(" Languages"));
        msg.setText(tr("New language setting will take effect upon restart.") + QString("\r\nNew language setting will take effect upon restart."));
    }
    msg.exec();
}

void MainWindow::addFavouriteItem(int type, const QString & name, const QString & description)
{
    if(type == 0) // Tools
    {
        char buffer[MAX_SETTING_SIZE];
        unsigned int i;
        for(i = 1; BridgeSettingGet("Favourite", (QString("Tool") + QString::number(i)).toUtf8().constData(), buffer); i++)
        {
        }
        BridgeSettingSet("Favourite", (QString("Tool") + QString::number(i)).toUtf8().constData(), name.toUtf8().constData());
        BridgeSettingSet("Favourite", (QString("ToolDescription") + QString::number(i)).toUtf8().constData(), description.toUtf8().constData());
        if(BridgeSettingGet("Favourite", (QString("Tool") + QString::number(i + 1)).toUtf8().constData(), buffer))
        {
            buffer[0] = 0;
            BridgeSettingSet("Favourite", (QString("Tool") + QString::number(i + 1)).toUtf8().constData(), buffer);
        }
        updateFavouriteTools();
    }
    else if(type == 2) // Commands
    {
        char buffer[MAX_SETTING_SIZE];
        unsigned int i;
        for(i = 1; BridgeSettingGet("Favourite", (QString("Command") + QString::number(i)).toUtf8().constData(), buffer); i++)
        {
        }
        BridgeSettingSet("Favourite", (QString("Command") + QString::number(i)).toUtf8().constData(), name.toUtf8().constData());
        BridgeSettingSet("Favourite", (QString("CommandShortcut") + QString::number(i)).toUtf8().constData(), description.toUtf8().constData());
        if(BridgeSettingGet("Favourite", (QString("Command") + QString::number(i + 1)).toUtf8().constData(), buffer))
        {
            buffer[0] = 0;
            BridgeSettingSet("Favourite", (QString("Command") + QString::number(i + 1)).toUtf8().constData(), buffer);
        }
        updateFavouriteTools();
    }
}

void MainWindow::setFavouriteItemShortcut(int type, const QString & name, const QString & shortcut)
{
    if(type == 0)
    {
        char buffer[MAX_SETTING_SIZE];
        for(unsigned int i = 1; BridgeSettingGet("Favourite", QString("Tool%1").arg(i).toUtf8().constData(), buffer); i++)
        {
            if(QString(buffer) == name)
            {
                BridgeSettingSet("Favourite", (QString("ToolShortcut") + QString::number(i)).toUtf8().constData(), shortcut.toUtf8().constData());
                updateFavouriteTools();
                break;
            }
        }
    }
}

void MainWindow::animateIntoSlot()
{
    if(DbgIsDebugging())
        DbgFunctions()->AnimateCommand("StepInto;AnimateWait");
}

void MainWindow::animateOverSlot()
{
    if(DbgIsDebugging())
        DbgFunctions()->AnimateCommand("StepOver;AnimateWait");
}

void MainWindow::animateCommandSlot()
{
    QString command;
    if(SimpleInputBox(this, tr("Animate command"), "", command, tr("Example: StepInto")))
        DbgFunctions()->AnimateCommand(command.toUtf8().constData());
}

void MainWindow::setInitializationScript()
{
    QString global, debuggee;
    char globalChar[MAX_SETTING_SIZE];
    if(DbgIsDebugging())
    {
        debuggee = QString(DbgFunctions()->DbgGetDebuggeeInitScript());
        BrowseDialog browseScript(this, tr("Set Initialization Script for Debuggee"), tr("Set Initialization Script for Debuggee"), tr("Script files (*.txt *.scr);;All files (*.*)"), debuggee, false);
        browseScript.setWindowIcon(DIcon("initscript.png"));
        if(browseScript.exec() == QDialog::Accepted)
            DbgFunctions()->DbgSetDebuggeeInitScript(browseScript.path.toUtf8().constData());
    }
    if(BridgeSettingGet("Engine", "InitializeScript", globalChar))
        global = QString(globalChar);
    else
        global = QString();
    BrowseDialog browseScript(this, tr("Set Global Initialization Script"), tr("Set Global Initialization Script"), tr("Script files (*.txt *.scr);;All files (*.*)"), global, false);
    browseScript.setWindowIcon(DIcon("initscript.png"));
    if(browseScript.exec() == QDialog::Accepted)
    {
        BridgeSettingSet("Engine", "InitializeScript", browseScript.path.toUtf8().constData());
    }
}

void MainWindow::customizeMenu()
{
    CustomizeMenuDialog customMenuDialog(this);
    customMenuDialog.setWindowTitle(tr("Customize Menus"));
    customMenuDialog.setWindowIcon(DIcon("analysis.png"));
    customMenuDialog.exec();
    onMenuCustomized();
}

#include "../src/bridge/Utf8Ini.h"

void MainWindow::on_actionImportSettings_triggered()
{
    auto filename = QFileDialog::getOpenFileName(this, tr("Open file"), QCoreApplication::applicationDirPath(), tr("Settings (*.ini);;All files (*.*)"));
    if(!filename.length())
        return;
    QFile f(QDir::toNativeSeparators(filename));
    if(f.open(QFile::ReadOnly | QFile::Text))
    {
        QTextStream in(&f);
        auto style = in.readAll();
        f.close();
        Utf8Ini ini;
        int errorLine;
        if(ini.Deserialize(style.toStdString(), errorLine))
        {
            auto sections = ini.Sections();
            for(const auto & section : sections)
            {
                auto keys = ini.Keys(section);
                for(const auto & key : keys)
                    BridgeSettingSet(section.c_str(), key.c_str(), ini.GetValue(section, key).c_str());
            }
            Config()->load();
            DbgSettingsUpdated();
            emit Config()->colorsUpdated();
            emit Config()->fontsUpdated();
            emit Config()->shortcutsUpdated();
            emit Config()->tokenizerConfigUpdated();
            GuiUpdateAllViews();
        }
    }
}

void MainWindow::on_actionImportdatabase_triggered()
{
    if(!DbgIsDebugging())
        return;
    auto filename = QFileDialog::getOpenFileName(this, tr("Import database"), QString(), tr("Databases (%1);;All files (*.*)").arg(ArchValue("*.dd32", "*.dd64")));
    if(!filename.length())
        return;
    DbgCmdExec(QString("dbload \"%1\"").arg(QDir::toNativeSeparators(filename)).toUtf8().constData());
}

void MainWindow::on_actionExportdatabase_triggered()
{
    if(!DbgIsDebugging())
        return;
    auto filename = QFileDialog::getSaveFileName(this, tr("Export database"), QString(), tr("Databases (%1);;All files (*.*)").arg(ArchValue("*.dd32", "*.dd64")));
    if(!filename.length())
        return;
    DbgCmdExec(QString("dbsave \"%1\"").arg(QDir::toNativeSeparators(filename)).toUtf8().constData());
}

static void setupMenuCustomizationHelper(QMenu* parentMenu, QList<QAction*> & stringList)
{
    for(int i = 0; i < parentMenu->actions().size(); i++)
    {
        QAction* action = parentMenu->actions().at(i);
        stringList.append(action);
    }
}

void MainWindow::setupMenuCustomization()
{
    mFileMenuStrings.append(new QAction("File", this));
    setupMenuCustomizationHelper(ui->menuFile, mFileMenuStrings);
    mDebugMenuStrings.append(new QAction("Debug", this));
    setupMenuCustomizationHelper(ui->menuDebug, mDebugMenuStrings);
    mOptionsMenuStrings.append(new QAction("Option", this));
    setupMenuCustomizationHelper(ui->menuOptions, mOptionsMenuStrings);
    mHelpMenuStrings.append(new QAction("Help", this));
    setupMenuCustomizationHelper(ui->menuHelp, mHelpMenuStrings);
    mViewMenuStrings.append(new QAction("View", this));
    setupMenuCustomizationHelper(ui->menuView, mViewMenuStrings);
    onMenuCustomized();
    Config()->registerMainMenuStringList(&mFileMenuStrings);
    Config()->registerMainMenuStringList(&mDebugMenuStrings);
    Config()->registerMainMenuStringList(&mOptionsMenuStrings);
    Config()->registerMainMenuStringList(&mHelpMenuStrings);
    Config()->registerMainMenuStringList(&mViewMenuStrings);
}

void MainWindow::onMenuCustomized()
{
    QList<QMenu*> menus;
    QList<QString> menuNativeNames;
    QList<QList<QAction*>*> menuTextStrings;
    menus << ui->menuFile << ui->menuDebug << ui->menuOptions << ui->menuHelp << ui->menuView;
    menuNativeNames << "File" << "Debug" << "Option" << "Help" << "View";
    menuTextStrings << &mFileMenuStrings << &mDebugMenuStrings << &mOptionsMenuStrings << &mHelpMenuStrings << &mViewMenuStrings;
    for(int i = 0; i < menus.size(); i++)
    {
        QMenu* currentMenu = menus[i];
        QMenu* moreCommands = nullptr;
        bool moreCommandsUsed = false;
        QList<QAction*> list = currentMenu->actions();
        moreCommands = list.last()->menu();
        if(moreCommands && moreCommands->title().compare(tr("More Commands")) == 0)
        {
            for(auto & j : moreCommands->actions())
                moreCommands->removeAction(j);
            QAction* separatorMoreCommands = list.at(list.length() - 2);
            currentMenu->removeAction(separatorMoreCommands); // Separator
            delete separatorMoreCommands;
        }
        else
        {
            moreCommands = new QMenu(tr("More Commands"), currentMenu);
        }
        for(auto & j : list)
            currentMenu->removeAction(j);
        for(int j = 0; j < menuTextStrings.at(i)->size() - 1; j++)
        {
            QAction* a = menuTextStrings.at(i)->at(j + 1);
            if(Config()->getBool("Gui", QString("Menu%1Hidden%2").arg(menuNativeNames[i]).arg(j)))
            {
                moreCommands->addAction(a);
                moreCommandsUsed = true;
            }
            else
            {
                currentMenu->addAction(a);
            }
        }
        if(moreCommandsUsed)
        {
            currentMenu->addSeparator();
            currentMenu->addMenu(moreCommands);
        }
        else
            delete moreCommands;
    }
}

void MainWindow::on_actionRestartAdmin_triggered()
{
    DbgCmdExec("restartadmin");
}

void MainWindow::on_actionPlugins_triggered()
{
    QDesktopServices::openUrl(QUrl("http://plugins.x64dbg.com"));
}

void MainWindow::on_actionCheckUpdates_triggered()
{
    mUpdateChecker->checkForUpdates();
}
