#ifndef BREAKPOINTS_H
#define BREAKPOINTS_H

#include <QObject>
#include "Bridge.h"

enum BPXSTATE
{
    bp_enabled = 0,
    bp_disabled = 1,
    bp_non_existent = -1
};

class Breakpoints : public QObject
{
    Q_OBJECT
public:
    explicit Breakpoints(QObject* parent = 0);
    static void setBP(BPXTYPE type, uint_t va);
    static void enableBP(BRIDGEBP & bp);
    static void enableBP(BPXTYPE type, uint_t va);
    static void disableBP(BRIDGEBP & bp);
    static void disableBP(BPXTYPE type, uint_t va);
    static void removeBP(BRIDGEBP & bp);
    static void removeBP(BPXTYPE type, uint_t va);
    static void toggleBPByDisabling(BRIDGEBP & bp);
    static void toggleBPByDisabling(BPXTYPE type, uint_t va);
    static void toggleBPByRemoving(BPXTYPE type, uint_t va);
    static BPXSTATE BPState(BPXTYPE type, uint_t va);

};

#endif // BREAKPOINTS_H
