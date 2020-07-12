#ifndef LABELCLASS_H
#define LABELCLASS_H

#include <QLabel>
#include <QStatusBar>
#include "Bridge.h"

class DebugStatusLabel : public QLabel
{
    Q_OBJECT

public:
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    explicit DebugStatusLabel(QStatusBar* parent = 0);
    QString state() const;

public slots:
    void debugStateChangedSlot(DBGSTATE state);

signals:
    void stateChanged();

private:
    QString mStatusTexts[4];
    QString mState;
};

#endif // LABELCLASS_H
