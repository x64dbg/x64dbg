#include "ClickableMenuFilter.h"
#include <QEvent>
#include <QMouseEvent>
#include <QMenu>
#include <QTimer>

ClickableMenuFilter::ClickableMenuFilter(QObject* parent)
    : QObject(parent)
{
}

bool ClickableMenuFilter::eventFilter(QObject* watched, QEvent* event)
{
    if(event->type() == QEvent::MouseButtonRelease)
    {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if(mouseEvent->button() == Qt::LeftButton)
        {
            QMenu* menu = qobject_cast<QMenu*>(watched);
            QAction* action = menu->actionAt(mouseEvent->pos());
            QMenu* actionMenu = action->menu();
            if(action && actionMenu && !actionMenu->property("clickable").isNull())
            {
                action->trigger();
                menu->close();
                return true;
            }
        }
    }
    return QObject::eventFilter(watched, event);
}
