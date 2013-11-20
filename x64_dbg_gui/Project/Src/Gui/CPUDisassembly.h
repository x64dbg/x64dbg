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
    
signals:
    
public slots:
    void toggleInt3BPAction();
    void toggleHwBpActionSlot();
    void setNewOriginHereActionSlot();
    void gotoOrigin();
    void setLabel();
    void setComment();
    void setBookmark();

private:
    // Rigth Click Context Menu
    QMenu* mRightClickContextMenu;

    QAction* mToggleInt3BpAction;
    QAction* mToggleHwBpAction;
    QAction* mSetNewOriginHere;
    QAction* mGotoOrigin;
    QAction* mSetComment;
    QAction* mSetLabel;
    QAction* mSetBookmark;
};

#endif // CPUDISASSEMBLY_H
