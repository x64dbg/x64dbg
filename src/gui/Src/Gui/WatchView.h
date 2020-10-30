#ifndef WATCHVIEW_H
#define WATCHVIEW_H
#include "StdTable.h"

class CPUMultiDump;

class WatchView : public StdTable
{
    Q_OBJECT
public:
    WatchView(CPUMultiDump* parent);

    QString paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h) override;
    void updateColors() override;

public slots:
    void contextMenuSlot(const QPoint & event);
    void updateWatch();
    void addWatchSlot();
    void delWatchSlot();
    void renameWatchSlot();
    void editWatchSlot();
    void modifyWatchSlot();
    void watchdogDisableSlot();
    void watchdogChangedSlot();
    void watchdogUnchangedSlot();
    void watchdogIsTrueSlot();
    void watchdogIsFalseSlot();
    void setTypeUintSlot();
    void setTypeIntSlot();
    void setTypeFloatSlot();
    void setTypeAsciiSlot();
    void setTypeUnicodeSlot();

protected:
    void setupContextMenu();

    QString getSelectedId();

    MenuBuilder* mMenu;
    QPen mWatchTriggeredColor;
    QBrush mWatchTriggeredBackgroundColor;

private:
    enum
    {
        ColName = 0,
        ColExpr,
        ColValue,
        ColType,
        ColWatchdog,
        ColId
    };
};

#endif // WATCHVIEW_H
