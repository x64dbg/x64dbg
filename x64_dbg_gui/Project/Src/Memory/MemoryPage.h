#ifndef MEMORYPAGE_H
#define MEMORYPAGE_H

#include <QObject>
#include <QDebug>
#include <QtGui>
#include "NewTypes.h"
#include "windows.h"
#include "main.h"
#include "Bridge.h"

class MemoryPage : public QObject
{
    Q_OBJECT
public:
    explicit MemoryPage(uint_t parBase, uint_t parSize, QObject *parent = 0);

    void readOriginalMemory(byte_t* parDest, uint_t parRVA, uint_t parSize);
    uint_t getSize();
    uint_t getBase();
    void setAttributes(uint_t base, uint_t size);
    
signals:
    
public slots:

private:
    uint_t mBase;
    uint_t mSize;

    
};

#endif // MEMORYPAGE_H
