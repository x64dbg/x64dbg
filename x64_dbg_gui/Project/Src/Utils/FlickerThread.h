#ifndef FLICKERTHREAD_H
#define FLICKERTHREAD_H

#include <QThread>
#include <QWidget>

class FlickerThread : public QThread
{
    Q_OBJECT
public:
    explicit FlickerThread(QWidget* widget, QObject* parent = 0);

signals:
    void setStyleSheet(QString style);

private:
    void run();
    QWidget* mWidget;
};

#endif // FLICKERTHREAD_H
