#include "FlickerThread.h"
#include <QStyle>
#include <Windows.h>

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
        Sleep(delay);
        emit setStyleSheet(oldStyle);
        Sleep(delay);
    }
}
