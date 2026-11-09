#pragma once

#include "StdIconTable.h"
class CommonActions;

class CallStackView : public StdIconTable
{
    Q_OBJECT
public:
    explicit CallStackView(StdTable* parent = nullptr);
    void setupContextMenu();
    duint getSelectionVa();

protected:
    QString paintContent(QPainter* painter, duint row, duint col, int x, int y, int w, int h) override;
    QColor getCellColor(duint row, duint col) override;
    void updateColors() override;

protected slots:
    void updateCallStackSlot();
    void contextMenuSlot(const QPoint pos);
    void followAddressSlot();
    void followToSlot();
    void followFromSlot();
    void showSuspectedCallStackSlot();
    void followInThreadsSlot();
    void renameThreadSlot();
    void loadSymbolsForThreadSlot();
    void highlightByModuleSlot();
    void highlightBySymbolSlot();
    void clearHighlightSlot();

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

    enum HighlightMode
    {
        HighlightNone,
        HighlightModule,
        HighlightSymbol
    };

    HighlightMode mHighlightMode;
    QString mHighlightKey;
    QColor mUserColor;

    MenuBuilder* mMenuBuilder;
    CommonActions* mCommonActions;
    bool isSelectionValid();
    bool isThreadHeaderSelected();
    bool rowMatchesHighlight(duint row);
    void switchThread();
};
