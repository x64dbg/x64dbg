#ifndef CALLSTACKVIEW_H
#define CALLSTACKVIEW_H

#include "StdTable.h"

class CallStackView : public StdTable
{
    Q_OBJECT
public:
    explicit CallStackView(StdTable* parent = 0);
    void setupContextMenu();

protected:
    QString paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h) override;

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
        ColThread = 0,
        ColAddress,
        ColTo,
        ColFrom,
        ColSize,
        ColComment,
        ColParty
    };

    MenuBuilder* mMenuBuilder;
};

#endif // CALLSTACKVIEW_H
