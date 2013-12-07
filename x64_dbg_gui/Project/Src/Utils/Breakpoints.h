#ifndef BREAKPOINTS_H
#define BREAKPOINTS_H

#include <QObject>
#include "Bridge.h"
#include <QDebug>

class Breakpoints : public QObject
{
    Q_OBJECT
public:
    explicit Breakpoints(QObject *parent = 0);

    static void setBP(BPXTYPE type, uint_t va);

    static void enableBP(BRIDGEBP bp);
    static void enableBP(BPXTYPE type, uint_t va);

    static void disableBP(BRIDGEBP bp);
    static void disableBP(BPXTYPE type, uint_t va);

    static void removeBP(BRIDGEBP bp);
    static void removeBP(BPXTYPE type, uint_t va);

    static void toogleBPByDisabling(BRIDGEBP bp);
    static void toogleBPByDisabling(BPXTYPE type, uint_t va);

    static void toogleBPByRemoving(BPXTYPE type, uint_t va);
    
signals:
    
public slots:

private:
    
};

#endif // BREAKPOINTS_H
