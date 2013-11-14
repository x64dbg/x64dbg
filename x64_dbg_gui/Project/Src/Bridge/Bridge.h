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
    void readProcessMemory(byte_t* dest, uint_t va, uint_t size);
    uint_t getSize(uint_t va);
    void emitDisassembleAtSignal(int_t va, int_t eip);
    void emitDbgStateChanged(DBGSTATE state);
    uint_t getBase(uint_t addr);
    static Bridge* getBridge();
    static void initBridge();
    bool execCmd(const char* cmd);
    bool getMemMapFromDbg(MEMMAP* parMemMap);
    bool isValidExpression(const char* expression);
    void Free(void* ptr);
    bool getRegDumpFromDbg(REGDUMP* parRegDump);
    uint_t getValFromString(const char* string);
    bool valToString(const char* name, uint_t value);
    void emitAddMsgToLog(QString msg);
    void emitClearLog();
    void emitUpdateRegisters();
    
signals:
    void disassembleAt(int_t va, int_t eip);
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
