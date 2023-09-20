#pragma once

#include <QLabel>
#include <QStatusBar>
#include "Bridge.h"

class DebugStatusLabel : public QLabel
{
    Q_OBJECT

public:
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    explicit DebugStatusLabel(QStatusBar* parent = nullptr);
    QString state() const;

public slots:
    void debugStateChangedSlot(DBGSTATE state);

signals:
    void stateChanged();

private:
    QString mStatusTexts[4];
    QString mState;
};
