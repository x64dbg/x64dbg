#ifndef MAINWINDOWCLOSETHREAD_H
#define MAINWINDOWCLOSETHREAD_H

#include <QThread>

class MainWindowCloseThread : public QThread
{
    Q_OBJECT
signals:
    void canClose();

private:
    void run();
};

#endif // MAINWINDOWCLOSETHREAD_H
