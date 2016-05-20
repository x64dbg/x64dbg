#include "TimeWastedCounter.h"
#include "Bridge.h"

TimeWastedCounter::TimeWastedCounter(QObject* parent, QLabel* label)
    : QObject(parent), mLabel(label)
{
    mLabel->setFrameStyle(QFrame::Sunken | QFrame::Panel); //sunken style
    mLabel->setStyleSheet("QLabel { background-color : #c0c0c0; }");
    connect(Bridge::getBridge(), SIGNAL(updateTimeWastedCounter()), this, SLOT(updateTimeWastedCounter()));
}

void TimeWastedCounter::updateTimeWastedCounter()
{
    duint timeWasted = DbgGetTimeWastedCounter();
    int days = timeWasted / (60 * 60 * 24);
    int hours = (timeWasted / (60 * 60)) % 24;
    int minutes = (timeWasted / 60) % 60;
    int seconds = timeWasted % 60;
    mLabel->setText(tr("Time Wasted Debugging:") + QString().sprintf(" %d:%02d:%02d:%02d", days, hours, minutes, seconds));
}
