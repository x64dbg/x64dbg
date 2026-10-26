#pragma once

#include <BasicView/StdIconTable.h>

#include "core/DbgAdapter.h"

class QAction;
class QHideEvent;
class QMenu;
class QProgressDialog;
class QShowEvent;

class MemoryMapView : public StdIconTable
{
    Q_OBJECT
public:
    explicit MemoryMapView(DbgAdapter* adapter, QWidget* parent = nullptr);

    QString paintContent(QPainter* painter, duint row, duint col, int x, int y, int w, int h) override;

signals:
    void followDisasmRequested(duint address);
    void followDumpRequested(duint address);

protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

public slots:
    void onMemoryMapUpdated(const QVector<DbgMemoryPage> & pages);
    void onSessionEnded();

private slots:
    void onFollow();
    void onFollowDisassembly();
    void onFollowDump();
    void onDumpMemory();
    void onDumpProgress(duint done, duint total);
    void onDumpFinished();
    void onSwitchView();
    void onGotoRip();
    void onSelectionChanged();
    // Reselects the region holding the selected start, since stacks and heaps resize.
    void restoreSelection();
    void onContextMenu(const QPoint & pos);

private:
    enum
    {
        ColAddress = 0,
        ColSize,
        ColParty,
        ColInfo,
        ColContent,
        ColType,
        ColProtection,
    };

    void setupContextMenu();
    bool rowHolds(duint row, duint address);
    // Returns getRowCount() when no row holds the address.
    duint findRow(duint address);

    DbgAdapter* mAdapter = nullptr;
    duint mRip = 0;
    duint mSelectionStart = 0;
    duint mSelectionCount = 0;
    SortData mSelectionSort;
    QMenu* mContextMenu = nullptr;
    QMenu* mCopyMenu = nullptr;
    QAction* mDumpMemoryAction = nullptr;
    QAction* mSwitchViewAction = nullptr;
    QAction* mGotoRipAction = nullptr;
    QProgressDialog* mDumpProgress = nullptr;
};
