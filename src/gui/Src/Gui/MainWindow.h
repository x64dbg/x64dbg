#pragma once

#include <QMainWindow>
#include "Imports.h"

class QMutex;
class QDragEnterEvent;
class QDropEvent;
class QMutex;
class CloseDialog;
class CommandLineEdit;
class MHTabWidget;
class CPUWidget;
class MemoryMapView;
class CallStackView;
class SEHChainView;
class LogView;
class SymbolView;
class BreakpointsView;
class ScriptView;
class ReferenceManager;
class ThreadView;
class PatchDialog;
class CalculatorDialog;
class DebugStatusLabel;
class LogStatusLabel;
class SourceViewerManager;
class HandlesView;
class MainWindowCloseThread;
class TimeWastedCounter;
class NotesManager;
class SettingsDialog;
class SimpleTraceDialog;
class MRUList;
class UpdateChecker;
class TraceManager;

namespace Ui
{
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    void setupCommandBar();
    void setupStatusBar();
    void closeEvent(QCloseEvent* event);
    void setTab(QWidget* widget);
    void loadTabDefaultOrder();
    void loadTabSavedOrder();
    void clearTabWidget();

    static void loadSelectedTheme(bool reloadOnlyStyleCss = false);
    static void updateDarkTitleBar(QWidget* widget);

public slots:
    void saveWindowSettings();
    void loadWindowSettings();
    void executeCommand();
    void execCommandSlot();
    void setFocusToCommandBar();
    void displayMemMapWidget();
    void displayLogWidget();
    void displayScriptWidget();
    void displayAboutWidget();
    void execTocnd();
    void execTicnd();
    void animateIntoSlot();
    void animateOverSlot();
    void animateCommandSlot();
    void openFileSlot();
    void openRecentFileSlot(QString filename);
    void restartDebugging();
    void displayBreakpointWidget();
    void updateWindowTitleSlot(QString filename);
    void runSlot();
    void displayThreadsWidget();
    void displayCpuWidget();
    void displayCpuWidgetShowCpu();
    void displaySymbolWidget();
    void displaySourceViewWidget();
    void displayReferencesWidget();
    void displayVariables();
    void displayGraphWidget();
    void displayTraceWidget();
    void openSettings();
    void openAppearance();
    void openCalculator();
    void addRecentFile(QString file);
    void setLastException(unsigned int exceptionCode);
    void findStrings();
    void findModularCalls();
    void addMenuToList(QWidget* parent, QMenu* menu, GUIMENUTYPE hMenu, int hParentMenu = -1);
    void addMenu(int hMenu, QString title);
    void addMenuEntry(int hMenu, QString title);
    void addSeparator(int hMenu);
    void clearMenu(int hMenu, bool erase);
    void menuEntrySlot();
    void removeMenuEntry(int hEntryMenu);
    void setIconMenuEntry(int hEntry, QIcon icon);
    void setIconMenu(int hMenu, QIcon icon);
    void setCheckedMenuEntry(int hEntry, bool checked);
    void setHotkeyMenuEntry(int hEntry, QString hotkey, QString id);
    void setVisibleMenuEntry(int hEntry, bool visible);
    void setVisibleMenu(int hMenu, bool visible);
    void setNameMenuEntry(int hEntry, QString name);
    void setNameMenu(int hMenu, QString name);
    void runSelection();
    void runExpression();
    void getStrWindow(const QString title, QString* text);
    void patchWindow();
    void displayComments();
    void displayLabels();
    void displayBookmarks();
    void displayFunctions();
    void crashDump();
    void displayCallstack();
    void displaySEHChain();
    void setGlobalShortcut(QAction* action, const QKeySequence & key);
    void refreshShortcuts();
    void openShortcuts();
    void changeTopmost(bool checked);
    void mnemonicHelp();
    void donate();
    void blog();
    void reportBug();
    void displayAttach();
    void changeCommandLine();
    void displayManual();
    void canClose();
    void addQWidgetTab(QWidget* qWidget, QString nativeName);
    void addQWidgetTab(QWidget* qWidget);
    void showQWidgetTab(QWidget* qWidget);
    void closeQWidgetTab(QWidget* qWidget);
    void executeOnGuiThread(void* cbGuiThread, void* userdata);
    void tabMovedSlot(int from, int to);
    void chkSaveloadTabSavedOrderStateChangedSlot(bool state);
    void dbgStateChangedSlot(DBGSTATE state);
    void displayNotesWidget();
    void displayHandlesWidget();
    void manageFavourites();
    void updateFavouriteTools();
    void clickFavouriteTool();
    void chooseLanguage();
    void setInitializationScript();
    void customizeMenu();
    void addFavouriteItem(int type, const QString & name, const QString & description);
    void setFavouriteItemShortcut(int type, const QString & name, const QString & shortcut);
    void themeTriggeredSlot();

private:
    Ui::MainWindow* ui;
    CloseDialog* mCloseDialog;
    CommandLineEdit* mCmdLineEdit;
    MHTabWidget* mTabWidget;
    CPUWidget* mCpuWidget;
    MemoryMapView* mMemMapView;
    CallStackView* mCallStackView;
    SEHChainView* mSEHChainView;
    LogView* mLogView;
    SymbolView* mSymbolView;
    SourceViewerManager* mSourceViewManager;
    BreakpointsView* mBreakpointsView;
    ScriptView* mScriptView;
    ReferenceManager* mReferenceManager;
    ThreadView* mThreadView;
    PatchDialog* mPatchDialog;
    CalculatorDialog* mCalculatorDialog;
    HandlesView* mHandlesView;
    NotesManager* mNotesManager;
    TraceManager* mTraceWidget;
    SimpleTraceDialog* mSimpleTraceDialog;
    UpdateChecker* mUpdateChecker;
    DebugStatusLabel* mStatusLabel;
    LogStatusLabel* mLastLogLabel;
    QToolBar* mFavouriteToolbar;

    TimeWastedCounter* mTimeWastedCounter;

    QString mWindowMainTitle;

    MRUList* mMRUList;
    unsigned int lastException;

    QAction* actionManageFavourites;

    void updateMRUMenu();
    void setupLanguagesMenu();
    void setupThemesMenu();
    void onMenuCustomized();
    void setupMenuCustomization();
    QAction* makeCommandAction(QAction* action, const QString & command);

    //lists for menu customization
    QList<QAction*> mFileMenuStrings;
    QList<QAction*> mViewMenuStrings;
    QList<QAction*> mDebugMenuStrings;
    //"Plugins" menu cannot be customized for item hiding.
    //"Favourites" menu cannot be customized for item hiding.
    QList<QAction*> mOptionsMenuStrings;
    QList<QAction*> mHelpMenuStrings;

    //menu api
    struct MenuEntryInfo
    {
        MenuEntryInfo() = default;

        QAction* mAction = nullptr;
        int hEntry = -1;
        int hParentMenu = -1;
        QString hotkey;
        QString hotkeyId;
        bool hotkeyGlobal = false;
        bool deleted = false;
    };

    struct MenuInfo
    {
    public:
        MenuInfo(QWidget* parent, QMenu* mMenu, int hMenu, int hParentMenu, bool globalMenu)
            : parent(parent), mMenu(mMenu), hMenu(hMenu), hParentMenu(hParentMenu), globalMenu(globalMenu)
        {
        }

        MenuInfo() = default;

        QWidget* parent = nullptr;
        QMenu* mMenu = nullptr;
        int hMenu = -1;
        int hParentMenu = -1;
        bool globalMenu = false;
        bool deleted = false;
    };

    QMutex* mMenuMutex = nullptr;
    int hEntryMenuPool;
    QList<MenuEntryInfo> mEntryList;
    QList<MenuInfo> mMenuList;

    void initMenuApi();
    MenuInfo* findMenu(int hMenu);
    MenuEntryInfo* findMenuEntry(int hEntry);
    QString nestedMenuDescription(const MenuInfo* menu);
    QString nestedMenuEntryDescription(const MenuEntryInfo & entry);
    void clearMenuHelper(int hMenu, bool markAsDeleted);
    void clearMenuImpl(int hMenu, bool erase);

    bool bCanClose;
    bool bExitWhenDetached;
    MainWindowCloseThread* mCloseThread;

    struct WidgetInfo
    {
    public:
        WidgetInfo(QWidget* widget, QString nativeName)
        {
            this->widget = widget;
            this->nativeName = nativeName;
        }

        QWidget* widget;
        QString nativeName;
    };

    QList<WidgetInfo> mWidgetList;
    QList<WidgetInfo> mPluginWidgetList;

protected:
    void dragEnterEvent(QDragEnterEvent* pEvent) override;
    void dropEvent(QDropEvent* pEvent) override;
    bool event(QEvent* event) override;

private slots:
    void setupLanguagesMenu2();
    void updateStyle();

    void on_actionFaq_triggered();
    void on_actionReloadStylesheet_triggered();
    void on_actionImportSettings_triggered();
    void on_actionImportdatabase_triggered();
    void on_actionExportdatabase_triggered();
    void on_actionPlugins_triggered();
    void on_actionCheckUpdates_triggered();
    void on_actionDefaultTheme_triggered();
    void on_actionAbout_Qt_triggered();
};
