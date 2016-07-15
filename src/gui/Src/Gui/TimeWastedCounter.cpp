#include "TimeWastedCounter.h"
#include "Bridge.h"

TimeWastedCounter::TimeWastedCounter(QWidget* parent)
    : QLabel(parent)
{
    setFrameStyle(QFrame::Sunken | QFrame::Panel); //sunken style
    setStyleSheet("QLabel { background-color : #c0c0c0; }");
    mLastTime = GetTickCount64();
    BridgeSettingGetUint("TimeWastedCounter", "CounterMode", (duint*)&mCounterMode);
    connect(Bridge::getBridge(), SIGNAL(updateTimeWastedCounter()), this, SLOT(updateTimeWastedCounter()));
}

void TimeWastedCounter::mousePressEvent(QMouseEvent*)
{
    mCounterMode = (TimeWastedCounterMode::t)((int)mCounterMode + 1);
    if(mCounterMode >= TimeWastedCounterMode::SIZE)
    {
        mCounterMode = (TimeWastedCounterMode::t)0;
    }
    BridgeSettingSetUint("TimeWastedCounter", "CounterMode", mCounterMode);
}

void TimeWastedCounter::updateTimeWastedCounter()
{
    uint64 totalDbgEvents = DbgFunctions()->GetDbgEvents();
    uint64 dbgEvents = totalDbgEvents - mLastEventCount;
    mLastEventCount = totalDbgEvents;

    uint64 time = GetTickCount64();
    uint64 timeDelta = time - mLastTime;
    mLastTime = time;

    if(dbgEvents > 4)
    {
        QString msg = "";
        switch(mCounterMode)
        {
        default:
        case TimeWastedCounterMode::EventsDelta:
            msg = tr("%1 events in %2ms (%3ms)")
                  .arg(dbgEvents)
                  .arg(timeDelta);
            break;
        case TimeWastedCounterMode::EventsTotal:
            msg = tr("%1(+%2) total events (%3ms)")
                  .arg(totalDbgEvents)
                  .arg(dbgEvents);
            break;
        case TimeWastedCounterMode::EventsHertz:
            if(timeDelta == 0)  // Keeps the old message
                return;
            msg = tr("%1 events/sec (%3ms)")
                  .arg(dbgEvents * 1000 / timeDelta);
            break;
        }
        msg = msg.arg(Bridge::getBridge()->latencyMs);
        setText(msg);
    }
    else
    {
        duint timeWasted = DbgGetTimeWastedCounter();
        int days = timeWasted / (60 * 60 * 24);
        int hours = (timeWasted / (60 * 60)) % 24;
        int minutes = (timeWasted / 60) % 60;
        int seconds = timeWasted % 60;
        setText(tr("Time Wasted Debugging:") + QString().sprintf(" %d:%02d:%02d:%02d", days, hours, minutes, seconds));
    }
}
