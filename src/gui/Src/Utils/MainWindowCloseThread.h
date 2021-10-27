#pragma once

#include <QThread>

class MainWindowCloseThread : public QThread
{
    Q_OBJECT
public:
    explicit MainWindowCloseThread(QObject* parent = nullptr);

signals:
    void canClose();

private:
    void run();
};
