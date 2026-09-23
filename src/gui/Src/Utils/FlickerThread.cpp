#include "FlickerThread.h"
#include <QStyle>

FlickerThread::FlickerThread(QWidget* widget, QObject* parent) : QThread(parent)
{
    mWidget = widget;
    setProperties();
}

void FlickerThread::setProperties(int count, int width, int delay)
{
    this->count = count;
    this->width = width;
    this->delay = delay;
}

void FlickerThread::run()
{
    QString oldStyle = mWidget->styleSheet();
    for(int i = 0; i < count; i++)
    {
        emit setStyleSheet(QString("QWidget { border: %1px solid red; }").arg(width));
        msleep(delay);
        emit setStyleSheet(oldStyle);
        msleep(delay);
    }
}
