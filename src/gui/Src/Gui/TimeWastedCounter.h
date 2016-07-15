#ifndef TIMEWASTEDCOUNTER_H
#define TIMEWASTEDCOUNTER_H

#include <QObject>
#include <QLabel>
#include <stdint.h>

class TimeWastedCounter : public QLabel
{
    Q_OBJECT
public:
    explicit TimeWastedCounter(QWidget* parent);

private slots:
    void updateTimeWastedCounter();
protected:
    void mousePressEvent(QMouseEvent* event);
private:
    struct TimeWastedCounterMode
    {
        enum t
        {
            EventsDelta = 0,
            EventsTotal,
            EventsHertz,
            SIZE
        };
    };

    TimeWastedCounterMode::t mCounterMode = TimeWastedCounterMode::EventsDelta;
    uint64_t mLastEventCount = 0;
    uint64_t mLastTime = 0;
};

#endif // TIMEWASTEDCOUNTER_H
