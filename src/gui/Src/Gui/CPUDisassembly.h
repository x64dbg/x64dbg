#ifndef CPUDISASSEMBLY_H
#define CPUDISASSEMBLY_H

#include "Disassembly.h"
#include "GotoDialog.h"
#include "SourceViewerManager.h"

class CPUDisassembly : public Disassembly
{
    Q_OBJECT
public:
    explicit CPUDisassembly(QWidget* parent = 0);

    // Mouse Management
    void contextMenuEvent(QContextMenuEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void mouseDoubleClickEvent(QMouseEvent* event);

    // Context Menu Management
    void setupRightClickContextMenu();
    void addFollowReferenceMenuItem(QString name, int_t value, QMenu* menu, bool isReferences);
    void setupFollowReferenceMenu(int_t wVA, QMenu* menu, bool isReferences);
    void setHwBpAt(uint_t va, int slot);

    void copySelection(bool copyBytes);

signals:
    void displayReferencesWidget();
    void displaySourceManagerWidget();
    void showPatches();
    void decompileAt(int_t start, int_t end);
    void displaySnowmanWidget();

public slots:
    void refreshShortcutsSlot();
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
    void toggleFunction();
    void assembleAt();
    void gotoExpression();
    void gotoFileOffset();
    void gotoStartSlot();
    void gotoEndSlot();
    void followActionSlot();
    void gotoPrevious();
    void gotoNext();
    void findReferences();
    void findConstant();
    void findStrings();
    void findCalls();
    void findPattern();
    void selectionGet(SELECTIONDATA* selection);
    void selectionSet(const SELECTIONDATA* selection);
    void enableHighlightingMode();
    void binaryEditSlot();
    void binaryFillSlot();
    void binaryFillNopsSlot();
    void binaryCopySlot();
    void binaryPasteSlot();
    void binaryPasteIgnoreSizeSlot();
    void undoSelectionSlot();
    void showPatchesSlot();
    void yaraSlot();
    void copySelection();
    void copySelectionNoBytes();
    void copyAddress();
    void copyRva();
    void copyDisassembly();
    void findCommand();
    void openSource();
    void decompileSelection();
    void decompileFunction();

private:
    // Menus
    QMenu* mBinaryMenu;
    QMenu* mGotoMenu;
    QMenu* mFollowMenu;
    QMenu* mBPMenu;
    QMenu* mHwSlotSelectMenu;
    QMenu* mReferencesMenu;
    QMenu* mSearchMenu;
    QMenu* mCopyMenu;
    QMenu* mPluginMenu;

    QAction* mBinaryEditAction;
    QAction* mBinaryFillAction;
    QAction* mBinaryFillNopsAction;
    QAction* mBinaryCopyAction;
    QAction* mBinaryPasteAction;
    QAction* mBinaryPasteIgnoreSizeAction;
    QAction* mUndoSelection;
    QAction* mToggleInt3BpAction;
    QAction* mSetHwBpAction;
    QAction* mClearHwBpAction;
    QAction* mSetNewOriginHere;
    QAction* mGotoOrigin;
    QAction* mSetComment;
    QAction* mSetLabel;
    QAction* mSetBookmark;
    QAction* mToggleFunction;
    QAction* mAssemble;
    QAction* msetHwBPOnSlot0Action;
    QAction* msetHwBPOnSlot1Action;
    QAction* msetHwBPOnSlot2Action;
    QAction* msetHwBPOnSlot3Action;
    QAction* mGotoExpression;
    QAction* mGotoFileOffset;
    QAction* mGotoPrevious;
    QAction* mGotoNext;
    QAction* mGotoStart;
    QAction* mGotoEnd;
    QAction* mReferenceSelectedAddress;
    QAction* mSearchCommand;
    QAction* mSearchConstant;
    QAction* mSearchStrings;
    QAction* mSearchCalls;
    QAction* mSearchPattern;
    QAction* mEnableHighlightingMode;
    QAction* mPatchesAction;
    QAction* mYaraAction;
    QAction* mCopySelection;
    QAction* mCopySelectionNoBytes;
    QAction* mCopyAddress;
    QAction* mCopyRva;
    QAction* mCopyDisassembly;
    QAction* mOpenSource;

    QMenu* mDecompileMenu;
    QAction* mDecompileSelection;
    QAction* mDecompileFunction;

    GotoDialog* mGoto;
};

#endif // CPUDISASSEMBLY_H
