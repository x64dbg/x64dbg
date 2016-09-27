#ifndef LOGSTATUSLABEL_H
#define LOGSTATUSLABEL_H

#include <QLabel>
#include <QStatusBar>
#include "Bridge.h"

class LogStatusLabel : public QLabel
{
    Q_OBJECT
public:
    explicit LogStatusLabel(QStatusBar* parent = 0);

public slots:
    void logUpdate(QString message);

private:
    QString labelText;
};

#endif // LOGSTATUSLABEL_H
