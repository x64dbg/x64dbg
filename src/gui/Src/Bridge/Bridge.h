#ifndef BRIDGE_H
#define BRIDGE_H

#include <agents.h>
#include <QObject>
#include <QWidget>
#include <QMutex>
#include "Imports.h"
#include "ReferenceManager.h"
#include "BridgeResult.h"

class Bridge : public QObject
{
    Q_OBJECT

    friend class BridgeResult;

public:
    explicit Bridge(QObject* parent = 0);
    ~Bridge();

    static Bridge* getBridge();
    static void initBridge();

    // Message processing function
    void* processMessage(GUIMSG type, void* param1, void* param2);

    // Misc functions
    static void CopyToClipboard(const QString & text);
    static void CopyToClipboard(const QString & text, const QString & htmlText);

    //result function
    void setResult(dsint result = 0);

    //helper functions
    void emitMenuAddToList(QWidget* parent, QMenu* menu, int hMenu, int hParentMenu = -1);
    void setDbgStopped();

    //Public variables
    void* winId = nullptr;
    ReferenceManager* referenceManager = nullptr;
    QWidget* snowmanView = nullptr;
    bool mIsRunning = false;

signals:
    void disassembleAt(dsint va, dsint eip);
    void repaintGui();
    void dbgStateChanged(DBGSTATE state);
    void addMsgToLog(QByteArray msg);
    void clearLog();
    void close();
    void updateRegisters();
    void updateBreakpoints();
    void updateWindowTitle(QString filename);
    void dumpAt(dsint va);
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
    void referenceSetRowCount(dsint count);
    void referenceSetCellContent(int r, int c, QString s);
    void referenceAddCommand(QString title, QString command);
    void referenceReloadData();
    void referenceSetSingleSelection(int index, bool scroll);
    void referenceSetProgress(int progress);
    void referenceSetCurrentTaskProgress(int progress, QString taskTitle);
    void referenceSetSearchStartCol(int col);
    void referenceInitialize(QString name);
    void stackDumpAt(duint va, duint csp);
    void updateDump();
    void updateThreads();
    void updateMemory();
    void addRecentFile(QString file);
    void setLastException(unsigned int exceptionCode);
    void menuAddMenuToList(QWidget* parent, QMenu* menu, int hMenu, int hParentMenu);
    void menuAddMenu(int hMenu, QString title);
    void menuAddMenuEntry(int hMenu, QString title);
    void menuAddSeparator(int hMenu);
    void menuClearMenu(int hMenu, bool erase);
    void menuRemoveMenuEntry(int hEntryMenu);
    void selectionDisasmGet(SELECTIONDATA* selection);
    void selectionDisasmSet(const SELECTIONDATA* selection);
    void selectionDumpGet(SELECTIONDATA* selection);
    void selectionDumpSet(const SELECTIONDATA* selection);
    void selectionStackGet(SELECTIONDATA* selection);
    void selectionStackSet(const SELECTIONDATA* selection);
    void selectionGraphGet(SELECTIONDATA* selection);
    void selectionMemmapGet(SELECTIONDATA* selection);
    void selectionSymmodGet(SELECTIONDATA* selection);
    void getStrWindow(const QString title, QString* text);
    void autoCompleteAddCmd(const QString cmd);
    void autoCompleteDelCmd(const QString cmd);
    void autoCompleteClearAll();
    void addMsgToStatusBar(QString msg);
    void updateSideBar();
    void repaintTableView();
    void updatePatches();
    void updateCallStack();
    void updateSEHChain();
    void updateArgumentView();
    void symbolRefreshCurrent();
    void loadSourceFile(const QString path, int line, int selection);
    void setIconMenuEntry(int hEntry, QIcon icon);
    void setIconMenu(int hMenu, QIcon icon);
    void setCheckedMenuEntry(int hEntry, bool checked);
    void setVisibleMenuEntry(int hEntry, bool visible);
    void setVisibleMenu(int hMenu, bool visible);
    void setNameMenuEntry(int hEntry, QString name);
    void setNameMenu(int hMenu, QString name);
    void setHotkeyMenuEntry(int hEntry, QString hotkey, QString id);
    void showCpu();
    void addQWidgetTab(QWidget* qWidget);
    void showQWidgetTab(QWidget* qWidget);
    void closeQWidgetTab(QWidget* qWidget);
    void executeOnGuiThread(void* cbGuiThread);
    void updateTimeWastedCounter();
    void setGlobalNotes(const QString text);
    void getGlobalNotes(void* text);
    void setDebuggeeNotes(const QString text);
    void getDebuggeeNotes(void* text);
    void dumpAtN(duint va, int index);
    void displayWarning(QString title, QString text);
    void registerScriptLang(SCRIPTTYPEINFO* info);
    void unregisterScriptLang(int id);
    void focusDisasm();
    void focusDump();
    void focusStack();
    void focusGraph();
    void focusMemmap();
    void updateWatch();
    void loadGraph(BridgeCFGraphList* graph, duint addr);
    void graphAt(duint addr);
    void updateGraph();
    void setLogEnabled(bool enabled);
    void addFavouriteItem(int type, const QString & name, const QString & description);
    void setFavouriteItemShortcut(int type, const QString & name, const QString & shortcut);
    void foldDisassembly(duint startAddr, duint length);
    void selectInMemoryMap(duint addr);
    void getActiveView(ACTIVEVIEW* activeView);
    void addInfoLine(const QString & text);
    void typeAddNode(void* parent, const TYPEDESCRIPTOR* type);
    void typeClear();
    void typeUpdateWidget();
    void closeApplication();
    void flushLog();
    void getDumpAttention();
    void openTraceFile(const QString & fileName);
    void updateTraceBrowser();
    void symbolSelectModule(duint base);

private:
    CRITICAL_SECTION csBridge;
    HANDLE hResultEvent;
    DWORD dwMainThreadId = 0;
    dsint bridgeResult = 0;
    volatile bool dbgStopped = false;
};

void DbgCmdExec(const QString & cmd);
bool DbgCmdExecDirect(const QString & cmd);

#endif // BRIDGE_H
