#ifndef BRIDGE_H
#define BRIDGE_H

#include <QObject>
#include <QDebug>
#include <QtGui>
#include "NewTypes.h"

#include "main.h"

#include "Exports.h"
#include "Imports.h"


class Bridge : public QObject
{
    Q_OBJECT
public:
    explicit Bridge(QObject *parent = 0);

    static Bridge* getBridge();
    static void initBridge();

    // Exports Binding
    void emitDisassembleAtSignal(int_t va, int_t eip);
    void emitUpdateDisassembly();
    void emitDbgStateChanged(DBGSTATE state);
    void emitAddMsgToLog(QString msg);
    void emitClearLog();
    void emitUpdateRegisters();
    void emitUpdateBreakpoints();
    void emitUpdateWindowTitle(QString filename);
    void emitUpdateCPUTitle(QString modname);
    void emitSetInfoLine(int line, QString text);
    void emitClearInfoBox();
    void emitDumpAt(int_t va);
    void emitScriptAdd(int count, const char** lines);
    void emitScriptClear();
    void emitScriptSetIp(int line);
    void emitScriptError(int line, QString message);
    void emitScriptSetTitle(QString title);
    void emitScriptSetInfoLine(int line, QString info);
    void emitScriptMessage(QString message);
    int emitScriptQuestion(QString message);
    void emitUpdateSymbolList(int module_count, SYMBOLMODULEINFO* modules);
    void emitAddMsgToSymbolLog(QString msg);
    void emitClearSymbolLog();
    void emitSetSymbolProgress(int progress);

    void* winId;
    QWidget* scriptView;
    int scriptResult;
    
signals:
    void disassembleAt(int_t va, int_t eip);
    void repaintGui();
    void dbgStateChanged(DBGSTATE state);
    void addMsgToLog(QString msg);
    void clearLog();
    void updateRegisters();
    void updateBreakpoints();
    void updateWindowTitle(QString filename);
    void updateCPUTitle(QString modname);
    void setInfoLine(int line, QString text);
    void dumpAt(int_t va);

    void scriptAdd(int count, const char** lines);
    void scriptClear();
    void scriptSetIp(int line);
    void scriptError(int line, QString message);
    void scriptSetTitle(QString title);
    void scriptSetInfoLine(int line, QString info);
    void scriptMessage(QString message);
    void scriptQuestion(QString message);

    void updateSymbolList(int module_count, SYMBOLMODULEINFO* modules);
    void addMsgToSymbolLog(QString msg);
    void clearSymbolLog();
    void setSymbolProgress(int progress);
    
public slots:

private:
    QMutex mBridgeMutex;

public:

};

#endif // BRIDGE_H
