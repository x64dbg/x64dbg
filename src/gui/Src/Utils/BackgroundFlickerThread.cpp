#include "BackgroundFlickerThread.h"
#include "Configuration.h"
#include <Windows.h>

BackgroundFlickerThread::BackgroundFlickerThread(QWidget* widget, QColor & background, QObject* parent) : QThread(parent), background(background)
{
    mWidget = widget;
    setProperties();
}

void BackgroundFlickerThread::setProperties(int count, int delay)
{
    this->count = count;
    this->delay = delay;
}

void BackgroundFlickerThread::run()
{
    QColor flickerColor = ConfigColor("BackgroundFlickerColor");
    QColor oldColor = background;
    for(int i = 0; i < count; i++)
    {
        background = flickerColor;
        mWidget->update();
        Sleep(delay);

        background = oldColor;
        mWidget->update();
        Sleep(delay);
    }
}
