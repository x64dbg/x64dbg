#pragma once

#include "StdIconTable.h"
class CommonActions;

class CallStackView : public StdIconTable
{
    Q_OBJECT
public:
    explicit CallStackView(StdTable* parent = 0);
    void setupContextMenu();
    duint getSelectionVa();

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
        ColParty,
        ColComment
    };

    MenuBuilder* mMenuBuilder;
    CommonActions* mCommonActions;
    bool isSelectionValid();
};
