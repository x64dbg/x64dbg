#pragma once

#include "Disassembly.h"

// Needed forward declaration for parent container class
class CPUSideBar;
class GotoDialog;
class XrefBrowseDialog;
class CommonActions;

class CPUDisassembly : public Disassembly
{
    Q_OBJECT

public:
    CPUDisassembly(Architecture* architecture, bool isMain, QWidget* parent = nullptr);

    // Mouse management
    void contextMenuEvent(QContextMenuEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void mouseDoubleClickEvent(QMouseEvent* event);

    // Context menu management
    void setupRightClickContextMenu();
    void copySelectionSlot(bool copyBytes);
    void copySelectionToFileSlot(bool copyBytes);
    void setSideBar(CPUSideBar* sideBar);

signals:
    void displayReferencesWidget();
    void displaySourceManagerWidget();
    void showPatches();
    void displayLogWidget();
    void displaySymbolsWidget();

public slots:
    void gotoOriginSlot();
    void setLabelSlot();
    void setLabelAddressSlot();
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
    void traceCoverageBitSlot();
    void traceCoverageByteSlot();
    void traceCoverageWordSlot();
    void traceCoverageDisableSlot();
    void traceCoverageToggleTraceRecordingSlot();
    void displayWarningSlot(QString title, QString text);
    void labelHelpSlot();
    void analyzeSingleFunctionSlot();
    void removeAnalysisSelectionSlot();
    void removeAnalysisModuleSlot();
    void setEncodeTypeSlot();
    void setEncodeTypeRangeSlot();
    void analyzeModuleSlot();
    void copyTokenTextSlot();
    void copyTokenValueSlot();
    void downloadCurrentSymbolsSlot();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    int findDeepestLoopDepth(duint addr);
    bool getLabelsFromInstruction(duint addr, QSet<QString> & labels);
    bool getTokenValueText(QString & text);

    void pushSelectionInto(bool copyBytes, QTextStream & stream, QTextStream* htmlStream = nullptr);

    void addFollowReferenceMenuItem(QString name, duint value, QMenu* menu, bool isReferences, bool isFollowInCPU);
    void setupFollowReferenceMenu(duint va, QMenu* menu, bool isReferences, bool isFollowInCPU);

    // Menus
    QMenu* mHwSlotSelectMenu;
    QMenu* mPluginMenu = nullptr;

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

    QAction* mFindCommandAllUser;
    QAction* mFindConstantAllUser;
    QAction* mFindStringsAllUser;
    QAction* mFindCallsAllUser;
    QAction* mFindPatternAllUser;
    QAction* mFindGUIDAllUser;

    QAction* mFindCommandAllSystem;
    QAction* mFindConstantAllSystem;
    QAction* mFindStringsAllSystem;
    QAction* mFindCallsAllSystem;
    QAction* mFindPatternAllSystem;
    QAction* mFindGUIDAllSystem;

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
    CPUSideBar* mSideBar = nullptr;

    MenuBuilder* mMenuBuilder;
    MenuBuilder* mHighlightMenuBuilder;
    bool mHighlightContextMenu = false;
    CommonActions* mCommonActions;
};
