#include "FlickerThread.h"
#include <QStyle>
#include <Windows.h>

FlickerThread::FlickerThread(QWidget* widget, QObject* parent) : QThread(parent)
{
    mWidget = widget;
}

void FlickerThread::run()
{
    QString oldStyle = mWidget->styleSheet();
    int delay = 300;
    for(int i = 0; i < 3 ; i++)
    {
        emit setStyleSheet("QWidget { border: 2px solid red; }");
        Sleep(delay);
        emit setStyleSheet(oldStyle);
        Sleep(delay);
    }
}
