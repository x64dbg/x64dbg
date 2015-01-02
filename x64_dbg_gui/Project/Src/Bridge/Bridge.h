#ifndef BRIDGE_H
#define BRIDGE_H

#include <QObject>
#include <QMutex>
#include "Imports.h"
#include "NewTypes.h"
#include "ReferenceManager.h"

class Bridge : public QObject
{
    Q_OBJECT
public:
    explicit Bridge(QObject* parent = 0);
    ~Bridge();

    static Bridge* getBridge();
    static void initBridge();

    // Misc functions
    static void CopyToClipboard(const QString & text);

    //result function
    void BridgeSetResult(int_t result);

    // Exports Binding
    void emitDisassembleAtSignal(int_t va, int_t eip);
    void emitUpdateDisassembly();
    void emitDbgStateChanged(DBGSTATE state);
    void emitAddMsgToLog(QString msg);
    void emitClearLog();
    void emitUpdateRegisters();
    void emitUpdateBreakpoints();
    void emitUpdateWindowTitle(QString filename);
    void emitDumpAt(int_t va);
    void emitScriptAdd(int count, const char** lines);
    void emitScriptClear();
    void emitScriptSetIp(int line);
    void emitScriptError(int line, QString message);
    void emitScriptSetTitle(QString title);
    void emitScriptSetInfoLine(int line, QString info);
    void emitScriptMessage(QString message);
    int emitScriptQuestion(QString message);
    void emitScriptEnableHighlighting(bool enable);
    void emitUpdateSymbolList(int module_count, SYMBOLMODULEINFO* modules);
    void emitAddMsgToSymbolLog(QString msg);
    void emitClearSymbolLog();
    void emitSetSymbolProgress(int progress);
    void emitReferenceAddColumnAt(int width, QString title);
    void emitReferenceSetRowCount(int_t count);
    void emitReferenceSetCellContent(int r, int c, QString s);
    void emitReferenceReloadData();
    void emitReferenceSetSingleSelection(int index, bool scroll);
    void emitReferenceSetProgress(int progress);
    void emitReferenceSetSearchStartCol(int col);
    void emitReferenceInitialize(QString name);
    void emitStackDumpAt(uint_t va, uint_t csp);
    void emitUpdateDump();
    void emitUpdateThreads();
    void emitUpdateMemory();
    void emitAddRecentFile(QString file);
    void emitSetLastException(unsigned int exceptionCode);
    int emitMenuAddMenu(int hMenu, QString title);
    int emitMenuAddMenuEntry(int hMenu, QString title);
    void emitMenuAddSeparator(int hMenu);
    void emitMenuClearMenu(int hMenu);
    bool emitSelectionGet(int hWindow, SELECTIONDATA* selection);
    bool emitSelectionSet(int hWindow, const SELECTIONDATA* selection);
    bool emitGetStrWindow(const QString title, QString* text);
    void emitAutoCompleteAddCmd(const QString cmd);
    void emitAutoCompleteDelCmd(const QString cmd);
    void emitAutoCompleteClearAll();
    void emitAddMsgToStatusBar(QString msg);
    void emitUpdateSideBar();
    void emitRepaintTableView();
    void emitUpdatePatches();
    void emitUpdateCallStack();
    void emitSymbolRefreshCurrent();

    //Public variables
    void* winId;
    QWidget* scriptView;
    ReferenceManager* referenceManager;

signals:
    void disassembleAt(int_t va, int_t eip);
    void repaintGui();
    void dbgStateChanged(DBGSTATE state);
    void addMsgToLog(QString msg);
    void clearLog();
    void updateRegisters();
    void updateBreakpoints();
    void updateWindowTitle(QString filename);
    void dumpAt(int_t va);
    void scriptAdd(int count, const char** lines);
    void scriptClear();
    void scriptSetIp(int line);
    void scriptError(int line, QString message);
    void scriptSetTitle(QString title);
    void scriptSetInfoLine(int line, QString info);
    void scriptMessage(QString message);
    void scriptQuestion(QString message);
    void scriptEnableHighlighting(bool enable);
    void updateSymbolList(int module_count, SYMBOLMODULEINFO* modules);
    void addMsgToSymbolLog(QString msg);
    void clearSymbolLog();
    void setSymbolProgress(int progress);
    void referenceAddColumnAt(int width, QString title);
    void referenceSetRowCount(int_t count);
    void referenceSetCellContent(int r, int c, QString s);
    void referenceReloadData();
    void referenceSetSingleSelection(int index, bool scroll);
    void referenceSetProgress(int progress);
    void referenceSetSearchStartCol(int col);
    void referenceInitialize(QString name);
    void stackDumpAt(uint_t va, uint_t csp);
    void updateDump();
    void updateThreads();
    void updateMemory();
    void addRecentFile(QString file);
    void setLastException(unsigned int exceptionCode);
    void menuAddMenu(int hMenu, QString title);
    void menuAddMenuEntry(int hMenu, QString title);
    void menuAddSeparator(int hMenu);
    void menuClearMenu(int hMenu);
    void selectionDisasmGet(SELECTIONDATA* selection);
    void selectionDisasmSet(const SELECTIONDATA* selection);
    void selectionDumpGet(SELECTIONDATA* selection);
    void selectionDumpSet(const SELECTIONDATA* selection);
    void selectionStackGet(SELECTIONDATA* selection);
    void selectionStackSet(const SELECTIONDATA* selection);
    void getStrWindow(const QString title, QString* text);
    void autoCompleteAddCmd(const QString cmd);
    void autoCompleteDelCmd(const QString cmd);
    void autoCompleteClearAll();
    void addMsgToStatusBar(QString msg);
    void updateSideBar();
    void repaintTableView();
    void updatePatches();
    void updateCallStack();
    void symbolRefreshCurrent();

private:
    QMutex* mBridgeMutex;
    int_t bridgeResult;
    bool hasBridgeResult;

public:

};

#endif // BRIDGE_H
