#ifndef QBEAENGINE_H
#define QBEAENGINE_H

#include <QtGui>
#include <QDebug>

#define BEA_ENGINE_STATIC
#include "BeaEngine.h"
#include "NewTypes.h"

typedef struct _Instruction_t
{
    QString instStr;
    QByteArray dump;
    uint_t rva;
    int lentgh;
    DISASM disasm;
} Instruction_t;



class QBeaEngine
{

public:
    explicit QBeaEngine();

    ulong DisassembleBack(byte_t *data, uint_t base, uint_t size, uint_t ip, int n);
    ulong DisassembleNext(byte_t *data, uint_t base, uint_t size, uint_t ip, int n);
    Instruction_t DisassembleAt(byte_t* data, uint_t size, uint_t instIndex, uint_t origBase, uint_t origInstRVA);
signals:
    
public slots:


private:
    DISASM mDisasmStruct;
    
};

#endif // QBEAENGINE_H
