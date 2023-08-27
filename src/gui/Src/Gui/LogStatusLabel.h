#pragma once

#include <QLabel>
#include "Bridge.h"

class QStatusBar;

class LogStatusLabel : public QLabel
{
    Q_OBJECT
public:
    explicit LogStatusLabel(QStatusBar* parent = nullptr);

public slots:
    void logUpdate(QString message, bool encodeHTML = true);
    void logUpdateUtf8(QByteArray message);
    void logUpdateUtf8Html(QByteArray message);
    void focusChanged(QWidget* old, QWidget* now);
    void getActiveView(ACTIVEVIEW* active);
    // show status tip
    void showMessage(const QString & message);
    void linkActivatedSlot(const QString & link);

private:
    QString finalLabel;
    QString labelText;
    QString statusTip;
};
