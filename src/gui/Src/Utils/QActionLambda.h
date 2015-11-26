#ifndef QACTIONLAMBDA
#define QACTIONLAMBDA

#include <QObject>
#include <functional>

class QActionLambda : public QObject
{
    Q_OBJECT
public:
    typedef std::function<void()> TriggerCallback;

    QActionLambda(QObject* parent, TriggerCallback callback)
        : QObject(parent),
          _callback(callback)
    {
    }

public slots:
    void triggeredSlot()
    {
        if(_callback)
            _callback();
    }

private:
    TriggerCallback _callback;
};

#endif // QACTIONLAMBDA

