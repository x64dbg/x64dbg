#ifndef CPUDISASSEMBLY_H
#define CPUDISASSEMBLY_H

#include <QtGui>
#include <QtDebug>
#include "NewTypes.h"
#include "Disassembly.h"
#include "Bridge.h"
#include "LineEditDialog.h"

class CPUDisassembly : public Disassembly
{
    Q_OBJECT
public:
    explicit CPUDisassembly(QWidget *parent = 0);

    // Mouse Management
    void contextMenuEvent(QContextMenuEvent* event);

    // Context Menu Management
    void setupRightClickContextMenu();

    void setHwBpAt(uint_t va, int slot);
    
signals:
    
public slots:
    void toggleInt3BPAction();
    void toggleHwBpActionSlot();
    void setHwBpOnSlot0ActionSlot();
    void setHwBpOnSlot1ActionSlot();
    void setHwBpOnSlot2ActionSlot();
    void setHwBpOnSlot3ActionSlot();
    void setNewOriginHereActionSlot();
    void gotoOrigin();
    void setLabel();
    void setComment();
    void setBookmark();

private:
    // Rigth Click Context Menu
    QMenu* mRightClickContextMenu;

    // Menus
    QMenu* mGotoMenu;
    QMenu* mBPMenu;
    QMenu* mHwSlotSelectMenu;

    QAction* mToggleInt3BpAction;
    QAction* mSetHwBpAction;
    QAction* mClearHwBpAction;
    QAction* mSetNewOriginHere;
    QAction* mGotoOrigin;
    QAction* mSetComment;
    QAction* mSetLabel;
    QAction* mSetBookmark;
    QAction* msetHwBPOnSlot0Action;
    QAction* msetHwBPOnSlot1Action;
    QAction* msetHwBPOnSlot2Action;
    QAction* msetHwBPOnSlot3Action;


};

#endif // CPUDISASSEMBLY_H
