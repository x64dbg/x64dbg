#ifndef LABELCLASS_H
#define LABELCLASS_H

#include <QLabel>
#include <QStatusBar>
#include "Bridge.h"

class DebugStatusLabel : public QLabel
{
    Q_OBJECT
public:
    explicit DebugStatusLabel(QStatusBar* parent = 0);

public slots:
    void debugStateChangedSlot(DBGSTATE state);

private:
    QString statusTexts[4];
};

#endif // LABELCLASS_H
