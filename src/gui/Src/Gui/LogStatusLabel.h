#pragma once

#include <QLabel>
#include "Bridge.h"

class QStatusBar;

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
    // show status tip
    void showMessage(const QString & message);

private:
    QString finalLabel;
    QString labelText;
    QString statusTip;
};
