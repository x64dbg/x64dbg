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
    void onIncrementAction();
    void onDecrementAction();
    void onIncrementx87StackAction();
    void onDecrementx87StackAction();
    void onZeroAction();
    void onSetToOneAction();
    void onModifyAction();
    void onToggleValueAction();
    void onUndoAction();
    void onCopyToClipboardAction();
    void onCopyFloatingPointToClipboardAction();
    void onCopySymbolToClipboardAction();
    void onFollowInDisassembly();
    void onFollowInDump();
    void onFollowInDumpN();
    void onFollowInStack();
    void onFollowInMemoryMap();
    void onRemoveHardware();
    void onIncrementPtrSize();
    void onDecrementPtrSize();
    void onPushAction();
    void onPopAction();
    void onHighlightSlot();
    // switch SIMD display modes
    void onSIMDMode();
    void onFpuMode();
    void ModifyFields(const QString & title, STRING_VALUE_TABLE_t* table, SIZE_T size);
    void disasmSelectionChangedSlot(dsint va);

private:
    void CreateDumpNMenu(QMenu* dumpMenu);
    void displayEditDialog();

    CPUWidget* mParent;
    // context menu actions
    QMenu* mSwitchSIMDDispMode;
    QAction* mDisplaySTX;
    QAction* mDisplayx87rX;
    QAction* mDisplayMMX;
    QAction* mFollowInDump;
    QAction* wCM_Increment;
    QAction* wCM_Decrement;
    QAction* wCM_IncrementPtrSize;
    QAction* wCM_DecrementPtrSize;
    QAction* wCM_Push;
    QAction* wCM_Pop;
    QAction* wCM_Zero;
    QAction* wCM_SetToOne;
    QAction* wCM_Modify;
    QAction* wCM_ToggleValue;
    QAction* wCM_Undo;
    QAction* wCM_CopyToClipboard;
    QAction* wCM_CopyFloatingPointValueToClipboard;
    QAction* wCM_CopySymbolToClipboard;
    QAction* wCM_CopyAll;
    QAction* wCM_FollowInDisassembly;
    QAction* wCM_FollowInDump;
    QAction* wCM_FollowInStack;
    QAction* wCM_FollowInMemoryMap;
    QAction* wCM_RemoveHardware;
    QAction* wCM_Incrementx87Stack;
    QAction* wCM_Decrementx87Stack;
    QAction* wCM_ChangeFPUView;
    QAction* wCM_Highlight;
    QAction* SIMDHex;
    QAction* SIMDFloat;
    QAction* SIMDDouble;
    QAction* SIMDSWord;
    QAction* SIMDUWord;
    QAction* SIMDHWord;
    QAction* SIMDSDWord;
    QAction* SIMDUDWord;
    QAction* SIMDHDWord;
    QAction* SIMDSQWord;
    QAction* SIMDUQWord;
    QAction* SIMDHQWord;
};

#endif // CPUREGISTERSVIEW_H
