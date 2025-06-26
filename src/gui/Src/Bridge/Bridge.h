#pragma once

#include <QObject>
#include <QWidget>
#include <QMutex>
#include <QMenu>
#include "Imports.h"
#include "BridgeResult.h"
#include "Architecture.h"

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
namespace Qt
{
    static QString::SplitBehavior KeepEmptyParts = QString::KeepEmptyParts;
    static QString::SplitBehavior SkipEmptyParts = QString::SkipEmptyParts;
}
#endif // QT_VERSION

class ReferenceManager;
class SymbolView;

class Bridge : public QObject
{
    Q_OBJECT

    friend class BridgeResult;

    void doUpdate(GUIMSG msg);

private slots:
    void throttleUpdateSlot(GUIMSG msg);

public:
    explicit Bridge(QObject* parent = nullptr);
    ~Bridge();

    static Bridge* getBridge();
    static void initBridge();
    static Architecture* getArchitecture();

    // Message processing function
    void* processMessage(GUIMSG type, void* param1, void* param2);

    // Misc functions
    static void CopyToClipboard(const QString & text);
    static void CopyToClipboard(const QString & text, const QString & htmlText);

    //result function
    void setResult(BridgeResult::Type type, dsint result = 0);

    //helper functions
    void emitMenuAddToList(QWidget* parent, QMenu* menu, GUIMENUTYPE hMenu, int hParentMenu = -1);
    void setDbgStopped();

    //Public variables
    void* mWinId = nullptr;
    ReferenceManager* mReferenceManager = nullptr;
    bool mIsRunning = false;
    duint mLastCip = 0;
    SymbolView* mSymbolView = nullptr;
    bool mLoggingEnabled = true;

signals:
    void disassembleAt(duint va, duint eip);
    void updateDisassembly();
    void dbgStateChanged(DBGSTATE state);
    void addMsgToLog(QByteArray msg);
    void addMsgToLogHtml(QByteArray msg);
    void clearLog();
    void saveLog();
    void saveLogToFile(QString file);
    void redirectLogStop();
    void redirectLogToFile(QString filename);
    void close();
    void updateRegisters();
    void updateBreakpoints();
    void updateWindowTitle(QString filename);
    void dumpAt(duint va);
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
    void referenceSetRowCount(duint count);
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
    void menuAddMenuToList(QWidget* parent, QMenu* menu, GUIMENUTYPE hMenu, int hParentMenu);
    void menuAddMenu(int hMenu, QString title);
    void menuAddMenuEntry(int hMenu, QString title);
    void menuAddSeparator(int hMenu);
    void menuClearMenu(int hMenu, bool erase);
    void menuRemoveMenuEntry(int hEntryMenu);
    void setIconMenuEntry(int hEntry, QIcon icon);
    void setIconMenu(int hMenu, QIcon icon);
    void setCheckedMenuEntry(int hEntry, bool checked);
    void setVisibleMenuEntry(int hEntry, bool visible);
    void setVisibleMenu(int hMenu, bool visible);
    void setNameMenuEntry(int hEntry, QString name);
    void setNameMenu(int hMenu, QString name);
    void setHotkeyMenuEntry(int hEntry, QString hotkey, QString id);
    void selectionDisasmGet(SELECTIONDATA* selection);
    void selectionDisasmSet(const SELECTIONDATA* selection);
    void selectionDumpGet(SELECTIONDATA* selection);
    void selectionDumpSet(const SELECTIONDATA* selection);
    void selectionStackGet(SELECTIONDATA* selection);
    void selectionStackSet(const SELECTIONDATA* selection);
    void selectionGraphGet(SELECTIONDATA* selection);
    void selectionMemmapGet(SELECTIONDATA* selection);
    void selectionMemmapSet(const SELECTIONDATA* selection);
    void selectionSymmodGet(SELECTIONDATA* selection);
    void selectionThreadsGet(SELECTIONDATA* selection);
    void selectionThreadsSet(const SELECTIONDATA* selection);
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
    void loadSourceFile(const QString path, duint addr);
    void showCpu();
    void showThreads();
    void addQWidgetTab(QWidget* qWidget);
    void showQWidgetTab(QWidget* qWidget);
    void closeQWidgetTab(QWidget* qWidget);
    void executeOnGuiThread(void* cbGuiThread, void* userdata);
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
    void focusStruct();
    void focusStack();
    void focusGraph();
    void focusMemmap();
    void focusSymmod();
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
    void typeVisit(QString typeName, duint addr);
    void typeListUpdated();
    void closeApplication();
    void flushLog();
    void getDumpAttention();
    void openTraceFile(const QString & fileName);
    void updateTraceBrowser();
    void symbolSelectModule(duint base);
    void getCurrentGraph(BridgeCFGraphList* graphList);
    void showReferences();
    void gotoTraceIndex(duint index);
    void showTraceBrowser();
    void throttleUpdate(GUIMSG msg);

private:
    CRITICAL_SECTION mCsBridge;
    HANDLE mResultEvents[BridgeResult::Last];
    duint mBridgeResults[BridgeResult::Last];
    DWORD mMainThreadId = 0;
    volatile bool mDbgStopped = false;
    QMap<GUIMSG, DWORD> mLastUpdates;
    QMap<GUIMSG, QTimer*> mUpdateTimers;
};
