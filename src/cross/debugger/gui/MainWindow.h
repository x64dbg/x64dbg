#pragma once

#include <QMainWindow>
#include <QTabWidget>
#include <QTextBrowser>
#include <BasicView/Disassembly.h>
#include <BasicView/HexDump.h>
#include "core/DbgAdapter.h"
#include "Gui/RegistersView.h"

class QThread;
class CPUStack;
class ThreadView;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onOpen();
    void onAttach();
    void onDetach();
    void onContinue() const;
    void onPause() const;
    void onStepInto() const;
    void onStepOver() const;
    void onToggleBreakpoint() const;
    void onProcessCreated(duint entryPoint);
    void onProcessExited(int exitCode) const;
    void onProcessDetached() const;
    void onStopped(duint rip, const QString & reason) const;
    void onLogMessage(const QString & msg) const;
    void onEngineError(const QString & error);
    void onSessionEnded() const;

private:
    bool endCurrentSession();
    void stopDebugThread();
    void detachDebugThread();
    void finishDebugThread();
    void setupToolBar();
    void setupTabs();
    QWidget* createCpuTab();
    void clearDebuggeeViews() const;

    DbgAdapter* mProvider = nullptr;
    QThread* mDebugThread = nullptr;
    QTabWidget* mTabWidget = nullptr;
    Disassembly* mDisassembly = nullptr;
    HexDump* mHexDump = nullptr;
    CPUStack* mStack = nullptr;
    ThreadView* mThreadView = nullptr;
    RegistersView* mRegisters = nullptr;
    QTextBrowser* mLog = nullptr;
    bool mSessionStartPending = false;
};
