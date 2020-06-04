#ifndef CPUDISASSEMBLY_H
#define CPUDISASSEMBLY_H

#include "Disassembly.h"
#include "BreakpointMenu.h"

// Needed forward declaration for parent container class
class CPUWidget;
class GotoDialog;
class XrefBrowseDialog;

class CPUDisassembly : public Disassembly
{
    Q_OBJECT

public:
    explicit CPUDisassembly(CPUWidget* parent);

    // Mouse management
    void contextMenuEvent(QContextMenuEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void mouseDoubleClickEvent(QMouseEvent* event);

    // Context menu management
    void setupRightClickContextMenu();
    void addFollowReferenceMenuItem(QString name, dsint value, QMenu* menu, bool isReferences, bool isFollowInCPU);
    void setupFollowReferenceMenu(dsint wVA, QMenu* menu, bool isReferences, bool isFollowInCPU);
    void copySelectionSlot(bool copyBytes);
    void copySelectionToFileSlot(bool copyBytes);

signals:
    void displayReferencesWidget();
    void displaySourceManagerWidget();
    void showPatches();
    void displayLogWidget();
    void displayGraphWidget();
    void displaySymbolsWidget();

public slots:
    void setNewOriginHereActionSlot();
    void gotoOriginSlot();
    void setLabelSlot();
    void setLabelAddressSlot();
    void setCommentSlot();
    void setBookmarkSlot();
    void toggleFunctionSlot();
    void toggleArgumentSlot();
    void addLoopSlot();
    void deleteLoopSlot();
    void assembleSlot();
    void gotoExpressionSlot();
    void gotoFileOffsetSlot();
    void gotoStartSlot();
    void gotoEndSlot();
    void gotoFunctionStartSlot();
    void gotoFunctionEndSlot();
    void gotoPreviousReferenceSlot();
    void gotoNextReferenceSlot();
    void followActionSlot();
    void gotoPreviousSlot();
    void gotoNextSlot();
    void gotoXrefSlot();
    void findReferencesSlot();
    void findConstantSlot();
    void findStringsSlot();
    void findCallsSlot();
    void findPatternSlot();
    void findGUIDSlot();
    void findNamesSlot();
    void selectionGetSlot(SELECTIONDATA* selection);
    void selectionSetSlot(const SELECTIONDATA* selection);
    void selectionUpdatedSlot();
    void enableHighlightingModeSlot();
    void binaryEditSlot();
    void binaryFillSlot();
    void binaryFillNopsSlot();
    void binaryCopySlot();
    void binaryPasteSlot();
    void binaryPasteIgnoreSizeSlot();
    void undoSelectionSlot();
    void showPatchesSlot();
    void copySelectionSlot();
    void copySelectionToFileSlot();
    void copySelectionNoBytesSlot();
    void copySelectionToFileNoBytesSlot();
    void copyAddressSlot();
    void copyRvaSlot();
    void copyFileOffsetSlot();
    void copyHeaderVaSlot();
    void copyDisassemblySlot();
    void labelCopySlot();
    void findCommandSlot();
    void openSourceSlot();
    void mnemonicHelpSlot();
    void mnemonicBriefSlot();
    void ActionTraceRecordBitSlot();
    void ActionTraceRecordByteSlot();
    void ActionTraceRecordWordSlot();
    void ActionTraceRecordDisableSlot();
    void ActionTraceRecordToggleRunTraceSlot();
    void displayWarningSlot(QString title, QString text);
    void labelHelpSlot();
    void analyzeSingleFunctionSlot();
    void removeAnalysisSelectionSlot();
    void removeAnalysisModuleSlot();
    void setEncodeTypeSlot();
    void setEncodeTypeRangeSlot();
    void graphSlot();
    void analyzeModuleSlot();
    //void togglePreviewSlot();
    void createThreadSlot();
    void copyTokenTextSlot();
    void copyTokenValueSlot();
    void followInMemoryMapSlot();
    void downloadCurrentSymbolsSlot();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    int findDeepestLoopDepth(duint addr);
    bool getLabelsFromInstruction(duint addr, QSet<QString> & labels);
    bool getTokenValueText(QString & text);

    void pushSelectionInto(bool copyBytes, QTextStream & stream, QTextStream* htmlStream = nullptr);

    // Menus
    QMenu* mHwSlotSelectMenu;
    QMenu* mPluginMenu;

    // Actions
    QAction* mReferenceSelectedAddressAction;
    QAction* mFindCommandRegion;
    QAction* mFindConstantRegion;
    QAction* mFindStringsRegion;
    QAction* mFindCallsRegion;
    QAction* mFindPatternRegion;
    QAction* mFindGUIDRegion;

    QAction* mFindCommandModule;
    QAction* mFindConstantModule;
    QAction* mFindStringsModule;
    QAction* mFindCallsModule;
    QAction* mFindPatternModule;
    QAction* mFindGUIDModule;
    QAction* mFindNamesModule;

    QAction* mFindCommandFunction;
    QAction* mFindConstantFunction;
    QAction* mFindStringsFunction;
    QAction* mFindCallsFunction;
    QAction* mFindPatternFunction;
    QAction* mFindGUIDFunction;

    QAction* mFindCommandAll;
    QAction* mFindConstantAll;
    QAction* mFindStringsAll;
    QAction* mFindCallsAll;
    QAction* mFindPatternAll;
    QAction* mFindGUIDAll;

    // Goto dialog specific
    GotoDialog* mGoto = nullptr;
    GotoDialog* mGotoOffset = nullptr;
    XrefBrowseDialog* mXrefDlg = nullptr;

    // Parent CPU window
    CPUWidget* mParentCPUWindow;

    MenuBuilder* mMenuBuilder;
    MenuBuilder* mHighlightMenuBuilder;
    bool mHighlightContextMenu = false;
    BreakpointMenu* mBreakpointMenu;
};

#endif // CPUDISASSEMBLY_H
