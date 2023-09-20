#pragma once

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
    explicit Breakpoints(QObject* parent = nullptr);
    static void setBP(BPXTYPE type, duint va);
    static void enableBP(const BRIDGEBP & bp);
    static void enableBP(BPXTYPE type, duint va);
    static void disableBP(const BRIDGEBP & bp);
    static void disableBP(BPXTYPE type, duint va);
    static void removeBP(const BRIDGEBP & bp);
    static void removeBP(BPXTYPE type, duint va);
    static void removeBP(const QString & DLLName);
    static void toggleBPByDisabling(const BRIDGEBP & bp);
    static void toggleBPByDisabling(BPXTYPE type, duint va);
    static void toggleBPByDisabling(const QString & DLLName);
    static void toggleAllBP(BPXTYPE type, bool bEnable);
    static void toggleBPByRemoving(BPXTYPE type, duint va);
    static BPXSTATE BPState(BPXTYPE type, duint va);
    static bool BPTrival(BPXTYPE type, duint va);
    static bool editBP(BPXTYPE type, const QString & addrText, QWidget* widget, const QString & createCommand = QString());
};
