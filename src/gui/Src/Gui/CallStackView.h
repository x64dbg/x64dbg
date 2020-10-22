#ifndef CALLSTACKVIEW_H
#define CALLSTACKVIEW_H

#include "StdTable.h"

class CallStackView : public StdTable
{
    Q_OBJECT
public:
    explicit CallStackView(StdTable* parent = 0);
    void setupContextMenu();

protected slots:
    void updateCallStack();
    void contextMenuSlot(const QPoint pos);
    void followAddress();
    void followTo();
    void followFrom();
    void showSuspectedCallStack();

private:
    enum
    {
        ColAddress = 0,
        ColTo,
        ColFrom,
        ColSize,
        ColComment,
        ColParty
    };

    MenuBuilder* mMenuBuilder;
};

#endif // CALLSTACKVIEW_H
