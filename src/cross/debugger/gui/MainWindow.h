#pragma once

#include <BasicView/Disassembly.h>
#include <BasicView/HexDump.h>
#include <Gui/RegistersView.h>
#include <QMainWindow>
#include <QTabWidget>
#include <QTextBrowser>

#include "core/DbgAdapter.h"

class QThread;
class CPUStack;
class ThreadView;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onOpen();
    void onAttach();
    void onDetach();
    void onContinue();
    void onPause();
    void onStepInto();
    void onStepOver();
    void onToggleBreakpoint();
    void onProcessCreated(duint entryPoint);
    void onProcessExited(int exitCode);
    void onProcessDetached();
    void onStopped(duint rip, const QString & reason);
    void onLogMessage(const QString & msg);
    void onEngineError(const QString & error);
    void onSessionEnded();

private:
    bool endCurrentSession();
    bool canStartSession();
    void stopDebugThread();
    void detachDebugThread();
    void finishDebugThread();
    void setupToolBar();
    void setupTabs();
    QWidget* createCpuTab();
    void clearDebuggeeViews();

    DbgAdapter* mProvider = nullptr;
    QThread* mDebugThread = nullptr;
    QThread* mRetiringThread = nullptr;
    QTabWidget* mTabWidget = nullptr;
    Disassembly* mDisassembly = nullptr;
    HexDump* mHexDump = nullptr;
    CPUStack* mStack = nullptr;
    ThreadView* mThreadView = nullptr;
    RegistersView* mRegisters = nullptr;
    QTextBrowser* mLog = nullptr;
    bool mSessionStartPending = false;
    // Terminating a process we only attached to is not ours to do by default.
    bool mAttachedSession = false;
};
