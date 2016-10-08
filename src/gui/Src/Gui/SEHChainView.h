#ifndef SEHCHAINVIEW_H
#define SEHCHAINVIEW_H

#include "StdTable.h"

class SEHChainView : public StdTable
{
    Q_OBJECT
public:
    explicit SEHChainView(StdTable* parent = 0);
    void setupContextMenu();

protected slots:
    void updateSEHChain();
    void contextMenuSlot(const QPoint pos);
    void doubleClickedSlot();
    void followAddress();
    void followHandler();

private:
    QAction* mFollowAddress;
    QAction* mFollowHandler;
};

#endif // SEHCHAINVIEW_H
