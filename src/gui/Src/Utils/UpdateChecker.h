#pragma once

#include <QThread>

class UpdateChecker : public QThread
{
    Q_OBJECT
public:
    UpdateChecker(QWidget* parent);
    void checkForUpdates();

signals:
    void updateCheckFinished(const QString & json);

protected:
    void run() override;

private slots:
    void finishedSlot(const QString & json);

private:
    QWidget* mParent = nullptr;
};
