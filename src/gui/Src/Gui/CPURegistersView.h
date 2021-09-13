#ifndef CPUREGISTERSVIEW_H
#define CPUREGISTERSVIEW_H

#include "RegistersView.h"

class CPURegistersView : public RegistersView
{
    Q_OBJECT
public:
    CPURegistersView(CPUWidget* parent = 0);

public slots:
    void setRegister(REGISTER_NAME reg, duint value);
    void updateRegistersSlot();
    virtual void debugStateChangedSlot(DBGSTATE state);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseDoubleClickEvent(QMouseEvent* event);
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void refreshShortcutsSlot();
    virtual void displayCustomContextMenuSlot(QPoint pos);

protected slots:
    void onIncrementx87StackAction();
    void onDecrementx87StackAction();
    void onModifyAction();
    void onToggleValueAction();
    void onUndoAction();
    void onFollowInDisassembly();
    void onFollowInDump();
    void onFollowInDumpN();
    void onFollowInStack();
    void onFollowInMemoryMap();
    void onRemoveHardware();
    void onHighlightSlot();
    void ModifyFields(const QString & title, STRING_VALUE_TABLE_t* table, SIZE_T size);
    void disasmSelectionChangedSlot(dsint va);

private:
    void CreateDumpNMenu(QMenu* dumpMenu);
    void displayEditDialog();

    CPUWidget* mParent;
    // context menu actions
    QAction* mFollowInDump;
    QAction* wCM_Modify;
    QAction* wCM_ToggleValue;
    QAction* wCM_Undo;
    QAction* wCM_FollowInDisassembly;
    QAction* wCM_FollowInDump;
    QAction* wCM_FollowInStack;
    QAction* wCM_FollowInMemoryMap;
    QAction* wCM_RemoveHardware;
    QAction* wCM_Incrementx87Stack;
    QAction* wCM_Decrementx87Stack;
    QAction* wCM_Highlight;
};

#endif // CPUREGISTERSVIEW_H
