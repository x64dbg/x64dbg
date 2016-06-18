#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDragEnterEvent>
#include <QComboBox>
#include "CloseDialog.h"
#include "CommandLineEdit.h"
#include "TabWidget.h"
#include "CPUWidget.h"
#include "MemoryMapView.h"
#include "CallStackView.h"
#include "SEHChainView.h"
#include "LogView.h"
#include "SymbolView.h"
#include "BreakpointsView.h"
#include "ScriptView.h"
#include "ReferenceManager.h"
#include "ThreadView.h"
#include "PatchDialog.h"
#include "CalculatorDialog.h"
#include "StatusLabel.h"
#include "UpdateChecker.h"
#include "SourceViewerManager.h"
#include "SnowmanView.h"
#include "HandlesView.h"
#include "MainWindowCloseThread.h"
#include "TimeWastedCounter.h"
#include "NotesManager.h"
#include "SettingsDialog.h"

namespace Ui
{
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = 0);
    ~MainWindow();

    void setupCommandBar();
    void setupStatusBar();
    void closeEvent(QCloseEvent* event);
    void setTab(QWidget* widget);
    void loadTabDefaultOrder();
    void loadTabSavedOrder();
    void clearTabWidget();

public slots:
    void executeCommand();
    void execStepOver();
    void execStepInto();
    void setFocusToCommandBar();
    void displayMemMapWidget();
    void displayLogWidget();
    void displayScriptWidget();
    void displayAboutWidget();
    void execClose();
    void execRun();
    void execRtr();
    void execRtu();
    void execTocnd();
    void execTicnd();
    void openFile();
    void execPause();
    void startScylla();
    void restartDebugging();
    void displayBreakpointWidget();
    void updateWindowTitleSlot(QString filename);
    void execeStepOver();
    void execeStepInto();
    void execeRun();
    void execeRtr();
    void execSkip();
    void execTRBit();
    void execTRByte();
    void execTRWord();
    void execTRNone();
    void execTRTIBT();
    void execTRTOBT();
    void execTRTIIT();
    void execTRTOIT();
    void displayCpuWidget();
    void displaySymbolWidget();
    void displaySourceViewWidget();
    void displayReferencesWidget();
    void displayThreadsWidget();
    void displaySnowmanWidget();
    void hideDebugger();
    void openSettings();
    void openAppearance();
    void openCalculator();
    void addRecentFile(QString file);
    void setLastException(unsigned int exceptionCode);
    void findStrings();
    void findModularCalls();
    void addMenuToList(QWidget* parent, QMenu* menu, int hMenu, int hParentMenu = -1);
    void addMenu(int hMenu, QString title);
    void addMenuEntry(int hMenu, QString title);
    void addSeparator(int hMenu);
    void clearMenu(int hMenu);
    void menuEntrySlot();
    void removeMenuEntry(int hEntry);
    void setIconMenuEntry(int hEntry, QIcon icon);
    void setIconMenu(int hMenu, QIcon icon);
    void runSelection();
    void getStrWindow(const QString title, QString* text);
    void patchWindow();
    void displayComments();
    void displayLabels();
    void displayBookmarks();
    void displayFunctions();
    void checkUpdates();
    void displayCallstack();
    void displaySEHChain();
    void setGlobalShortcut(QAction* action, const QKeySequence & key);
    void refreshShortcuts();
    void openShortcuts();
    void changeTopmost(bool checked);
    void donate();
    void reportBug();
    void displayAttach();
    void detach();
    void changeCommandLine();
    void displayManual();
    void decompileAt(dsint start, dsint end);
    void canClose();
    void addQWidgetTab(QWidget* qWidget, QString nativeName);
    void addQWidgetTab(QWidget* qWidget);
    void showQWidgetTab(QWidget* qWidget);
    void closeQWidgetTab(QWidget* qWidget);
    void executeOnGuiThread(void* cbGuiThread);
    void tabMovedSlot(int from, int to);
    void chkSaveloadTabSavedOrderStateChangedSlot(bool state);
    void dbgStateChangedSlot(DBGSTATE state);
    void displayNotesWidget();
    void displayHandlesWidget();

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
    SnowmanView* mSnowmanView;
    HandlesView* mHandlesView;
    NotesManager* mNotesManager;

    StatusLabel* mStatusLabel;
    StatusLabel* mLastLogLabel;

    UpdateChecker* mUpdateChecker;
    TimeWastedCounter* mTimeWastedCounter;

    QString mWindowMainTitle;

    QStringList mMRUList;
    int mMaxMRU;
    unsigned int lastException;

    void loadMRUList(int maxItems);
    void saveMRUList();
    void addMRUEntry(QString entry);
    void removeMRUEntry(QString entry);
    void updateMRUMenu();
    QString getMRUEntry(int index);

    //menu api
    struct MenuEntryInfo
    {
        QAction* mAction;
        int hEntry;
        int hParentMenu;
    };

    struct MenuInfo
    {
    public:
        MenuInfo(QWidget* parent, QMenu* mMenu, int hMenu, int hParentMenu)
        {
            this->parent = parent;
            this->mMenu = mMenu;
            this->hMenu = hMenu;
            this->hParentMenu = hParentMenu;
        }

        QWidget* parent;
        QMenu* mMenu;
        int hMenu;
        int hParentMenu;
    };

    QList<MenuEntryInfo> mEntryList;
    int hEntryNext;
    QList<MenuInfo> mMenuList;
    int hMenuNext;

    void initMenuApi();
    const MenuInfo* findMenu(int hMenu);

    bool bCanClose;
    MainWindowCloseThread* mCloseThread;
    QVector<QWidget*> mWidgetList;
    QVector<QString> mWidgetNativeNameList;

protected:
    void dragEnterEvent(QDragEnterEvent* pEvent);
    void dropEvent(QDropEvent* pEvent);

public:
    static QString windowTitle;

private slots:
    void on_actionFaq_triggered();
    void on_actionReloadStylesheet_triggered();
};

#endif // MAINWINDOW_H
