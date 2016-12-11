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
    void logUpdateUtf8(QByteArray message);
    void focusChanged(QWidget* old, QWidget* now);
    void getActiveView(ACTIVEVIEW* active);

private:
    QString labelText;
};

#endif // LOGSTATUSLABEL_H
