#include "gui/MainWindow.h"
#include <QMenuBar>
#include <QCheckBox>
#include <QMessageBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>
#include <QLabel>
#include <QThread>
#include <Memory/MemoryPage.h>
#include "core/LinuxArchitecture.h"
#include "gui/AttachDialog.h"
#include "gui/CPUStack.h"
#include "gui/ThreadView.h"

static LinuxArchitecture gArch;

static QIcon icon(const char* name)
{
    return QIcon(QString(":/Default/icons/%1.png").arg(name));
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("x64dbg");
    setWindowIcon(icon("bug"));
    resize(1200, 800);

    mProvider = new DbgAdapter(this);

    setupToolBar();
    setupTabs();

    connect(mProvider, &DbgAdapter::processCreated, this, &MainWindow::onProcessCreated, Qt::QueuedConnection);
    connect(mProvider, &DbgAdapter::processExited, this, &MainWindow::onProcessExited, Qt::QueuedConnection);
    connect(mProvider, &DbgAdapter::processDetached, this, &MainWindow::onProcessDetached, Qt::QueuedConnection);
    connect(mProvider, &DbgAdapter::sessionEnded, this, &MainWindow::onSessionEnded, Qt::QueuedConnection);
    connect(mProvider, &DbgAdapter::registersUpdated, this, [this](const REGDUMP & dump)
    {
        mRegisters->setRegisters(&dump);
    }, Qt::QueuedConnection);
    connect(mProvider, &DbgAdapter::logMessage, this, &MainWindow::onLogMessage, Qt::QueuedConnection);
    connect(mProvider, &DbgAdapter::errorMessage, this, &MainWindow::onEngineError, Qt::QueuedConnection);
    connect(mProvider, &DbgAdapter::stopped, this, &MainWindow::onStopped, Qt::QueuedConnection);

    const auto menuFile = menuBar()->addMenu(tr("&File"));
    const auto actionOpen = menuFile->addAction(tr("&Open..."), this, &MainWindow::onOpen);
    actionOpen->setShortcut(QKeySequence::Open);
    const auto actionAttach = menuFile->addAction(icon("attach"), tr("&Attach..."), this, &MainWindow::onAttach);
    actionAttach->setShortcut(ConfigShortcut("FileAttach"));
    const auto actionDetach = menuFile->addAction(icon("detach"), tr("&Detach"), this, &MainWindow::onDetach);
    actionDetach->setShortcut(ConfigShortcut("FileDetach"));
    menuFile->addSeparator();
    menuFile->addAction(tr("E&xit"), this, &QWidget::close);

    const auto menuView = menuBar()->addMenu(tr("&View"));
    const auto actionCpu = menuView->addAction(icon("processor-cpu"), tr("&CPU"), this, [this]
    {
        mTabWidget->setCurrentIndex(0);
    });
    actionCpu->setShortcut(ConfigShortcut("ViewCpu"));
    const auto actionLog = menuView->addAction(icon("log"), tr("&Log"), this, [this]
    {
        mTabWidget->setCurrentWidget(mLog);
    });
    actionLog->setShortcut(ConfigShortcut("ViewLog"));
    const auto actionThreads = menuView->addAction(icon("arrow-threads"), tr("&Threads"), this, [this]
    {
        mTabWidget->setCurrentWidget(mThreadView);
    });
    actionThreads->setShortcut(ConfigShortcut("ViewThreads"));

    const auto menuDebug = menuBar()->addMenu(tr("&Debug"));
    const auto actionRun = menuDebug->addAction(tr("&Run"), this, &MainWindow::onContinue);
    actionRun->setShortcut(Qt::Key_F9);

    menuDebug->addSeparator();
    const auto actionBp = menuDebug->addAction(tr("Toggle &Breakpoint"), this, &MainWindow::onToggleBreakpoint);
    actionBp->setShortcut(Qt::Key_F2);

    statusBar()->showMessage(tr("Ready"));
}

MainWindow::~MainWindow()
{
    stopDebugThread();
}

// A new session ends the live one. Windows asks first and offers to detach instead of
// terminating (AttachDialog::attachToProcess), and the choice is remembered in the same
// two settings, so the behaviour matches once a command layer wires cbDebugAttach up.
bool MainWindow::endCurrentSession()
{
    if(!mDebugThread)
        return true;

    if(!mProvider->isActive())
    {
        stopDebugThread();
        return true;
    }

    duint detachOnAttach = 0;
    if(ConfigBool("Gui", "ShowAttachConfirmation"))
    {
        const auto remember = new QCheckBox(tr("Remember my choice"));
        QMessageBox msgbox(this);
        msgbox.setIcon(QMessageBox::Question);
        msgbox.setWindowTitle(tr("Already debugging"));
        msgbox.setText(tr("You are already debugging a process. What would you like to do with the current process?"));

        msgbox.addButton(QMessageBox::Yes)->setText(tr("&Terminate"));
        msgbox.addButton(QMessageBox::No)->setText(tr("&Detach"));
        msgbox.addButton(QMessageBox::Cancel)->setText(tr("&Cancel"));
        msgbox.setDefaultButton(QMessageBox::Cancel);
        msgbox.setEscapeButton(QMessageBox::Cancel);
        msgbox.setCheckBox(remember);

        const int code = msgbox.exec();
        if(code == QMessageBox::Cancel)
            return false;

        detachOnAttach = code == QMessageBox::No ? 1 : 0;
        BridgeSettingSetUint("Engine", "DetachOnAttach", detachOnAttach);
        if(remember->isChecked())
            Config()->setBool("Gui", "ShowAttachConfirmation", false);
    }
    else
        BridgeSettingGetUint("Engine", "DetachOnAttach", &detachOnAttach);

    if(detachOnAttach)
        detachDebugThread();
    else
        stopDebugThread();
    return true;
}

void MainWindow::stopDebugThread()
{
    if(!mDebugThread)
        return;

    // Continue() breaks any pause spin-loop in the debug thread so
    // Stop()'s SIGKILL can be reaped by waitpid and the loop exits.
    mProvider->Continue();
    (void)mProvider->Stop();
    finishDebugThread();
}

void MainWindow::detachDebugThread()
{
    if(!mDebugThread)
        return;

    // Detach() services both a paused and a running loop, so no Continue() here:
    // resuming first would let the debuggee run on with our breakpoints still armed.
    mProvider->detach();
    finishDebugThread();
}

void MainWindow::finishDebugThread()
{
    if(!mDebugThread->wait(3000))
    {
        mDebugThread->terminate();
        mDebugThread->wait();
    }
    delete mDebugThread;
    mDebugThread = nullptr;
    mSessionStartPending = false;

    // wait() does not pump the event loop, so the session's queued sessionEnded is still
    // undelivered. Tear down here or it lands after the next session installs its provider.
    DbgSetMemoryProvider(nullptr);
    clearDebuggeeViews();
}

void MainWindow::setupToolBar()
{
    const auto toolBar = new QToolBar(tr("Main Toolbar"), this);
    toolBar->setMovable(false);
    toolBar->setIconSize(QSize(16, 16));
    toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    addToolBar(toolBar);

    const auto actionOpen = toolBar->addAction(icon("folder-horizontal-open"), QString(), this, &MainWindow::onOpen);
    actionOpen->setToolTip(tr("Open (Ctrl+O)"));

    const auto actionRestart = toolBar->addAction(icon("arrow-restart"), QString());
    actionRestart->setToolTip(tr("Restart"));
    actionRestart->setEnabled(false);

    const auto actionClose = toolBar->addAction(icon("control-stop"), QString());
    actionClose->setToolTip(tr("Close"));
    actionClose->setEnabled(false);

    toolBar->addSeparator();

    const auto actionRun = toolBar->addAction(icon("arrow-run"), QString(), this, &MainWindow::onContinue);
    actionRun->setToolTip(tr("Run (F9)"));

    const auto actionPause = toolBar->addAction(icon("control-pause"), QString(), this, &MainWindow::onPause);
    actionPause->setToolTip(tr("Pause"));

    toolBar->addSeparator();

    const auto actionStepInto = toolBar->addAction(icon("arrow-step-into"), QString(), this, &MainWindow::onStepInto);
    actionStepInto->setToolTip(tr("Step Into (F7)"));
    actionStepInto->setShortcut(Qt::Key_F7);

    const auto actionStepOver = toolBar->addAction(icon("arrow-step-over"), QString(), this, &MainWindow::onStepOver);
    actionStepOver->setToolTip(tr("Step Over (F8)"));
    actionStepOver->setShortcut(Qt::Key_F8);
}

void MainWindow::setupTabs()
{
    mTabWidget = new QTabWidget(this);
    mTabWidget->setDocumentMode(true);
    setCentralWidget(mTabWidget);

    mTabWidget->addTab(createCpuTab(), icon("processor-cpu"), tr("CPU"));

    mLog = new QTextBrowser(this);
    mLog->setAccessibleName(tr("Log"));
    mLog->setFont(ConfigFont("Log"));
    mTabWidget->addTab(mLog, icon("log"), tr("Log"));

    auto makePlaceholder = [this](const QString & text)
    {
        const auto label = new QLabel(text, this);
        label->setAlignment(Qt::AlignCenter);
        label->setFont(ConfigFont("Log"));
        return label;
    };

    mTabWidget->addTab(makePlaceholder(tr("Breakpoints view - not yet implemented")), icon("breakpoint"), tr("Breakpoints"));
    mTabWidget->addTab(makePlaceholder(tr("Memory map view - not yet implemented")), icon("memory-map"), tr("Memory Map"));
    mTabWidget->addTab(makePlaceholder(tr("Call stack view - not yet implemented")), icon("callstack"), tr("Call Stack"));
    mThreadView = new ThreadView(mProvider, this);
    mThreadView->setAccessibleName(tr("Threads"));
    mTabWidget->addTab(mThreadView, icon("arrow-threads"), tr("Threads"));

    onLogMessage("[x64dbg] Ready. Open an ELF binary to begin debugging.");
}

QWidget* MainWindow::createCpuTab()
{
    const auto memPage = new MemoryPage(0, 0, this);
    mDisassembly = new Disassembly(&gArch, false, this);
    mDisassembly->setAccessibleName(tr("Disassembly"));
    mHexDump = new HexDump(&gArch, this, memPage);
    mHexDump->setAccessibleName(tr("Dump"));
    mStack = new CPUStack(&gArch, mProvider, this);
    mStack->setAccessibleName(tr("Stack"));
    mRegisters = new RegistersView(this); // Sets its accessible name internally.

    {
        const int charwidth = mHexDump->getCharWidth();
        HexDump::ColumnDescriptor wColDesc;
        HexDump::DataDescriptor dDesc{};

        wColDesc.isData = true;
        wColDesc.itemCount = 16;
        wColDesc.separator = 4;
        dDesc.itemSize = HexDump::Byte;
        dDesc.byteMode = HexDump::HexByte;
        wColDesc.data = dDesc;
        mHexDump->appendResetDescriptor(8 + charwidth * 47, tr("Hex"), false, wColDesc);

        wColDesc.isData = true;
        wColDesc.itemCount = 16;
        wColDesc.separator = 0;
        dDesc.itemSize = HexDump::Byte;
        dDesc.byteMode = HexDump::AsciiByte;
        wColDesc.data = dDesc;
        mHexDump->appendDescriptor(8 + charwidth * 16, tr("ASCII"), false, wColDesc);

        wColDesc.isData = false;
        wColDesc.itemCount = 0;
        wColDesc.separator = 0;
        wColDesc.data = dDesc;
        mHexDump->appendDescriptor(0, "", false, wColDesc);
    }

    // Allow all widgets to be freely resized by splitters
    mDisassembly->setMinimumHeight(0);
    mHexDump->setMinimumHeight(0);
    mStack->setMinimumHeight(0);

    const auto topSplitter = new QSplitter(Qt::Horizontal);
    topSplitter->addWidget(mDisassembly);
    topSplitter->addWidget(mRegisters);
    topSplitter->setStretchFactor(0, 70);
    topSplitter->setStretchFactor(1, 30);

    const auto bottomSplitter = new QSplitter(Qt::Horizontal);
    bottomSplitter->addWidget(mHexDump);
    bottomSplitter->addWidget(mStack);
    bottomSplitter->setStretchFactor(0, 60);
    bottomSplitter->setStretchFactor(1, 40);

    const auto mainSplitter = new QSplitter(Qt::Vertical);
    mainSplitter->addWidget(topSplitter);
    mainSplitter->addWidget(bottomSplitter);
    mainSplitter->setStretchFactor(0, 55);
    mainSplitter->setStretchFactor(1, 45);
    mainSplitter->setChildrenCollapsible(false);

    connect(mStack, &CPUStack::followDisasmRequested,
            mDisassembly, &Disassembly::gotoAddress);

    return mainSplitter;
}

void MainWindow::onOpen()
{
    const auto path = QFileDialog::getOpenFileName(this, tr("Open ELF Binary"), QString(), tr("All Files (*)"));
    if(path.isEmpty())
        return;

    onLogMessage(QString("[x64dbg] Launching: %1").arg(path));

    const QFileInfo target(path);
    if(target.isFile() && !target.isExecutable())
    {
        if(target.isWritable() && QFile::setPermissions(path, QFile::permissions(path) | QFile::ExeOwner))
            onLogMessage(QString("[x64dbg] Added the execute bit to %1").arg(path));
        else
            onLogMessage(QString("[x64dbg] %1 is not executable, run chmod +x on it").arg(path));
    }

    if(!endCurrentSession())
        return;

    if(!mProvider->loadEngine())
        return;

    DbgSetMemoryProvider(mProvider);

    mSessionStartPending = true;

    //? Init and Start must run on the same thread
    auto pathBytes = path.toUtf8();
    mDebugThread = QThread::create([this, pathBytes]()
    {
        if(!mProvider->launch(pathBytes.constData()))
        {
            DbgSetMemoryProvider(nullptr);
            emit mProvider->logMessage("[x64dbg] Failed to launch process");
            return;
        }
        mProvider->Start();
        // A loop that never reached a session fires no terminal event, so nothing
        // else takes the provider back down.
        DbgSetMemoryProvider(nullptr);
    });
    mDebugThread->start();
}

void MainWindow::onAttach()
{
    AttachDialog dialog(this);
    if(dialog.exec() != QDialog::Accepted || dialog.selectedPid() <= 0)
        return;

    const pid_t pid = dialog.selectedPid();

    if(!endCurrentSession())
        return;

    if(!mProvider->loadEngine())
        return;

    DbgSetMemoryProvider(mProvider);

    // The attach runs on the debug thread, so its failure arrives as an error callback.
    mSessionStartPending = true;

    mDebugThread = QThread::create([this, pid]()
    {
        if(!mProvider->attach(pid))
        {
            DbgSetMemoryProvider(nullptr);
            return;
        }
        mProvider->Start();
        DbgSetMemoryProvider(nullptr);
    });
    mDebugThread->start();
}

void MainWindow::onDetach()
{
    if(!mDebugThread)
    {
        onLogMessage(QString("[x64dbg] %1").arg(tr("Not debugging anything")));
        return;
    }
    detachDebugThread();
}

void MainWindow::onContinue() const
{
    if(mProvider && mProvider->isActive() && mProvider->isPaused())
    {
        onLogMessage("[x64dbg] Resuming...");
        mProvider->Continue();
        statusBar()->showMessage(tr("Running"));
    }
}

void MainWindow::onProcessCreated(const duint entryPoint)
{
    mSessionStartPending = false;
    onLogMessage(QString("[x64dbg] Process attached, entry: 0x%1").arg(entryPoint, 0, 16));
    mHexDump->printDumpAt(entryPoint);
}

void MainWindow::onProcessExited(const int exitCode) const
{
    onLogMessage(QString("[x64dbg] Process exited with code %1").arg(exitCode));
    statusBar()->showMessage(QString("Process exited with code %1").arg(exitCode));
}

void MainWindow::onProcessDetached() const
{
    statusBar()->showMessage(tr("Detached"));
}

// The session is over however it ended. Leaving the provider installed keeps
// DbgIsDebugging() true, and the views act on a process that is gone.
void MainWindow::onSessionEnded() const
{
    DbgSetMemoryProvider(nullptr);
    clearDebuggeeViews();
}

void MainWindow::clearDebuggeeViews() const
{
    mDisassembly->reloadData();
    mHexDump->reloadData();
    constexpr REGDUMP emptyDump{};
    mRegisters->setRegisters(&emptyDump);
    // These two subscribe to sessionEnded themselves, but queued, and this runs on the
    // path where that signal is never delivered.
    mStack->onSessionEnded();
    mThreadView->onSessionEnded();
}

// A session that never starts reports only through this callback, and the log pane is
// not the tab the user is looking at when they asked for one.
void MainWindow::onEngineError(const QString & error)
{
    if(!mSessionStartPending)
        return;
    mSessionStartPending = false;
    QMessageBox::warning(this, tr("Cannot start debugging"), error);
}

void MainWindow::onLogMessage(const QString & msg) const
{
    mLog->append(msg);
}

void MainWindow::onPause() const
{
    if(mProvider && mProvider->isActive())
        mProvider->Pause();
}

void MainWindow::onStepInto() const
{
    if(mProvider && mProvider->isActive())
        mProvider->StepInto();
}

void MainWindow::onStepOver() const
{
    if(mProvider && mProvider->isActive())
        mProvider->StepOver();
}

void MainWindow::onToggleBreakpoint() const
{
    if(!mProvider || !mProvider->isActive())
        return;

    const duint addr = mDisassembly->rvaToVa(mDisassembly->getInitialSelection());
    if(!mProvider->toggleBreakpoint(addr))
        onLogMessage(QString("[x64dbg] Failed to toggle breakpoint at 0x%1").arg(addr, 0, 16));
    mDisassembly->reloadData();
}

void MainWindow::onStopped(const duint rip, const QString & reason) const
{
    mDisassembly->gotoAddress(rip);
    mDisassembly->reloadData();
    statusBar()->showMessage(QString("%1 - 0x%2").arg(reason).arg(rip, 0, 16));
    mTabWidget->setCurrentIndex(0);
}
