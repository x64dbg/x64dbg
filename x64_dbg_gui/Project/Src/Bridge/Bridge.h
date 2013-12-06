#ifndef BRIDGE_H
#define BRIDGE_H

#include <QObject>
#include <QDebug>
#include <QtGui>
#include "NewTypes.h"

#ifdef BUILD_LIB
    #include "Exports.h"
    #include "main.h"
#endif

#include "Imports.h"


class Bridge : public QObject
{
    Q_OBJECT
public:
    explicit Bridge(QObject *parent = 0);
    void emitDisassembleAtSignal(int_t va, int_t eip);
    void emitUpdateDisassembly();
    void emitDbgStateChanged(DBGSTATE state);
    static Bridge* getBridge();
    static void initBridge();
    void emitAddMsgToLog(QString msg);
    void emitClearLog();
    void emitUpdateRegisters();
    
signals:
    void disassembleAt(int_t va, int_t eip);
    void repaintGui();
    void dbgStateChanged(DBGSTATE state);
    void addMsgToLog(QString msg);
    void clearLog();
    void updateRegisters();
    
public slots:

private:

public:
    QByteArray* mData;
};

#endif // BRIDGE_H
