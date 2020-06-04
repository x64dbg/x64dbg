#include "TimeWastedCounter.h"
#include "Bridge.h"
#include <QLabel>

TimeWastedCounter::TimeWastedCounter(QObject* parent, QLabel* label)
    : QObject(parent), mLabel(label)
{
    connect(Bridge::getBridge(), SIGNAL(updateTimeWastedCounter()), this, SLOT(updateTimeWastedCounter()));
}

void TimeWastedCounter::updateTimeWastedCounter()
{
    duint dbgEvents = DbgFunctions()->GetDbgEvents();
    if(dbgEvents > 4)
    {
        mLabel->setText(tr("%1 events/s").arg(dbgEvents));
    }
    else
    {
        duint timeWasted = DbgGetTimeWastedCounter();
        int days = timeWasted / (60 * 60 * 24);
        int hours = (timeWasted / (60 * 60)) % 24;
        int minutes = (timeWasted / 60) % 60;
        int seconds = timeWasted % 60;
        mLabel->setText(tr("Time Wasted Debugging:") + QString().sprintf(" %d:%02d:%02d:%02d", days, hours, minutes, seconds));
    }
}
