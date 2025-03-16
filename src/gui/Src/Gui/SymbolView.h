#pragma once

#include <QWidget>
#include "Bridge.h"
#include "StdIconTable.h"

class QMenu;
class StdSearchListView;
class StdIconSearchListView;
class SearchListView;
class SymbolSearchList;
class QVBoxLayout;

namespace Ui
{
    class SymbolView;
}

class ModuleStdTable final : public StdIconTable
{
    Q_OBJECT

public:
    ModuleStdTable();
    void updateColors() override;
    QColor getCellColor(duint r, duint c) override;
    QString getCellContent(duint r, duint c) override;
    void sortRows(duint column, bool ascending) override;

private:
    MODULESYMBOLSTATUS getStatus(duint r);

    QColor mSymbolSystemTextColor;
    QColor mSymbolUserTextColor;
    QColor mSymbolUnloadedTextColor;
    QColor mSymbolLoadingTextColor;
    QColor mSymbolLoadedTextColor;
};

class SymbolView : public QWidget
{
    Q_OBJECT

public:
    explicit SymbolView(QWidget* parent = nullptr);
    ~SymbolView() override;
    void setupContextMenu();
    void saveWindowSettings();
    void loadWindowSettings();

    void invalidateSymbolSource(duint base);

private slots:
    void updateStyle();
    void reloadDataSlot();
    void addMsgToSymbolLogSlot(QString msg);
    void clearSymbolLogSlot();
    void moduleSelectionChanged(duint index);
    void updateSymbolList(int module_count, SYMBOLMODULEINFO* modules);
    void symbolFollow();
    void symbolFollowDump();
    void symbolFollowImport();
    void symbolSelectModule(duint base);
    void enterPressedSlot();
    void symbolContextMenu(QMenu* menu);
    void symbolRefreshCurrent();
    void labelHelpSlot();
    void moduleContextMenu(QMenu* menu);
    void moduleFollow();
    void moduleEntryFollow();
    void moduleDownloadSymbols();
    void moduleDownloadAllSymbols();
    void moduleCopyPath();
    void moduleBrowse();
    void moduleSetUser();
    void moduleSetSystem();
    void moduleSetParty();
    void moduleFollowMemMap();
    void toggleBreakpoint();
    void toggleBookmark();
    void refreshShortcutsSlot();
    void emptySearchResultSlot();
    void selectionGetSlot(SELECTIONDATA* selection);
    void moduleLoad();
    void moduleFree();

signals:
    void showReferences();

private:
    Ui::SymbolView* ui;
    QVBoxLayout* mMainLayout;
    QVBoxLayout* mSymbolLayout;
    QWidget* mSymbolPlaceHolder;
    SearchListView* mSymbolList;
    StdIconSearchListView* mModuleList;
    SymbolSearchList* mSymbolSearchList;
    QMap<QString, duint> mModuleBaseList;
    QAction* mFollowSymbolAction;
    QAction* mFollowSymbolDumpAction;
    QAction* mFollowSymbolImportAction;
    QAction* mToggleBreakpoint;
    QAction* mToggleBookmark;
    QAction* mFollowModuleAction;
    QAction* mFollowModuleEntryAction;
    QAction* mDownloadSymbolsAction;
    QAction* mDownloadAllSymbolsAction;
    QAction* mCopyPathAction;
    QAction* mModSetUserAction;
    QAction* mModSetSystemAction;
    QAction* mModSetPartyAction;
    QAction* mBrowseInExplorer;
    QAction* mFollowInMemMap;
    QAction* mLoadLib;
    QAction* mFreeLib;
    QAction* mLabelHelp;
    QMenu* mPluginMenu;

    static void cbSymbolEnum(SYMBOLINFO* symbol, void* user);
};
