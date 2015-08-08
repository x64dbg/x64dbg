#ifndef TIMEWASTEDCOUNTER_H
#define TIMEWASTEDCOUNTER_H

#include <QObject>
#include <QAction>

class TimeWastedCounter : public QObject
{
    Q_OBJECT
public:
    explicit TimeWastedCounter(QObject* parent, QAction* label);

private slots:
    void updateTimeWastedCounter();

private:
    QAction* mLabel;
};

#endif // TIMEWASTEDCOUNTER_H
