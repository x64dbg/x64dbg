#ifndef MEMORYMAP_H
#define MEMORYMAP_H

#include <QObject>
#include <QDebug>
#include <QtGui>
#include <QWidget>
#include "NewTypes.h"
#include "windows.h"

class ProcessMemoryMap : public QObject
{
    Q_OBJECT
public:

    explicit ProcessMemoryMap(QString fileName, QObject *parent = 0);
    QList<MEMORY_BASIC_INFORMATION> BruteForceProcessMemory();
    void printVirtualMemoryMap(QList<MEMORY_BASIC_INFORMATION> parList);

    
signals:
    
public slots:

private:
    HANDLE mhProcess;

    QByteArray mData;

    QList<QByteArray> mMemoryRegionsDataList;
    QList<MEMORY_BASIC_INFORMATION> mMemoryRegionsList;
    
};

#endif // MEMORYMAP_H
