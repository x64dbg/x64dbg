#ifndef BACKGROUNDFLICKERTHREAD_H
#define BACKGROUNDFLICKERTHREAD_H

#include <QThread>
#include <QWidget>
#include <QColor>

class BackgroundFlickerThread : public QThread
{
    Q_OBJECT
public:
    explicit BackgroundFlickerThread(QWidget* widget, QColor& background, QObject* parent = 0);
    void setProperties(int count = 3, int delay = 300);

private:
    void run();
    QWidget* mWidget;
    QColor& background;
    int count;
    int delay;
};

#endif // BACKGROUNDFLICKERTHREAD_H
