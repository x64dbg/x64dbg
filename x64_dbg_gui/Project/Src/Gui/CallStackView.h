#ifndef CALLSTACKVIEW_H
#define CALLSTACKVIEW_H

#include "StdTable.h"

class CallStackView : public StdTable
{
    Q_OBJECT
public:
    explicit CallStackView(StdTable* parent = 0);

signals:
    void showCpu();

protected slots:
    void updateCallStack();
};

#endif // CALLSTACKVIEW_H
