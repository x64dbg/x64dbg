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

    //result function
    void setResult(dsint result = 0);

    //helper functions
    void emitLoadSourceFile(const QString path, int line = 0, int selection = 0);
    void emitMenuAddToList(QWidget* parent, QMenu* menu, int hMenu, int hParentMenu = -1);
    void setDbgStopped();

    //Public variables
    void* winId;
    QWidget* scriptView;
    ReferenceManager* referenceManager;

signals:
    void disassembleAt(dsint va, dsint eip);
    void repaintGui();
    void dbgStateChanged(DBGSTATE state);
    void addMsgToLog(QString msg);
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
    void menuClearMenu(int hMenu);
    void menuRemoveMenuEntry(int hEntry);
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

private:
    QMutex* mBridgeMutex;
    dsint bridgeResult;
    volatile bool hasBridgeResult;
    volatile bool dbgStopped;
};

#endif // BRIDGE_H
