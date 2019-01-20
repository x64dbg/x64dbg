#ifndef CLICKABLEMENUFILTER_H
#define CLICKABLEMENUFILTER_H

#include <QObject>

class ClickableMenuFilter : public QObject
{
    Q_OBJECT

public:
    explicit ClickableMenuFilter(QObject* parent = nullptr);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
};

#endif //CLICKABLEMENUFILTER_H
