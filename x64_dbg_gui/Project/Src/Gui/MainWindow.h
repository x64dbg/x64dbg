#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtGui>
#include <QFileDialog>
#include <QMdiArea>
#include <QMdiSubWindow>
#include "CPUWidget.h"
#include "CommandLineEdit.h"
#include "MemoryMapView.h"
#include "LogView.h"
#include "StatusLabel.h"
#include "BreakpointsView.h"
#include "ScriptView.h"
#include "SymbolView.h"
#include "ReferenceView.h"
#include "ThreadView.h"
#include "SettingsDialog.h"
#include "tabwidget.h"
#include "Configuration.h"
#include "AppearanceDialog.h"
#include "ShortcutsDialog.h"
#include "CloseDialog.h"
#include "PatchDialog.h"
#include "UpdateChecker.h"
#include "CallStackView.h"
#include "CalculatorDialog.h"

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
    static DWORD WINAPI closeThread(void* ptr);
    void closeEvent(QCloseEvent* event);
    void setTab(QWidget* widget);
    bool bClose;

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
    void displayCpuWidget();
    void displaySymbolWidget();
    void displayReferencesWidget();
    void displayThreadsWidget();
    void openSettings();
    void openAppearance();
    void openCalculator();
    void addRecentFile(QString file);
    void setLastException(unsigned int exceptionCode);
    void findStrings();
    void findModularCalls();
    void addMenu(int hMenu, QString title);
    void addMenuEntry(int hMenu, QString title);
    void addSeparator(int hMenu);
    void clearMenu(int hMenu);
    void menuEntrySlot();
    void runSelection();
    void getStrWindow(const QString title, QString* text);
    void patchWindow();
    void displayComments();
    void displayLabels();
    void displayBookmarks();
    void displayFunctions();
    void checkUpdates();
    void displayCallstack();
    void refreshShortcuts();
    void openShortcuts();
    void donate();

private:
    Ui::MainWindow* ui;
    CloseDialog* mCloseDialog;

    CommandLineEdit* mCmdLineEdit;
    MHTabWidget* mTabWidget;
    CPUWidget* mCpuWidget;
    MemoryMapView* mMemMapView;
    CallStackView* mCallStackView;
    LogView* mLogView;
    SymbolView* mSymbolView;
    BreakpointsView* mBreakpointsView;
    ScriptView* mScriptView;
    ReferenceView* mReferenceView;
    ThreadView* mThreadView;
    PatchDialog* mPatchDialog;
    CalculatorDialog* mCalculatorDialog;

    StatusLabel* mStatusLabel;
    StatusLabel* mLastLogLabel;

    UpdateChecker* mUpdateChecker;

    const char* mWindowMainTitle;

    std::vector<QString> mMRUList;
    unsigned int mMaxMRU;
    unsigned int lastException;

    void loadMRUList(int maxItems);
    void saveMRUList();
    void addMRUEntry(QString entry);
    void removeMRUEntry(QString entry);
    void updateMRUMenu();
    QString getMRUEntry(size_t index);

    //menu api
    struct MenuEntryInfo
    {
        QAction* mAction;
        int hEntry;
        int hParentMenu;
    };

    struct MenuInfo
    {
        QMenu* mMenu;
        int hMenu;
        int hParentMenu;
    };

    QList<MenuEntryInfo> mEntryList;
    int hEntryNext;
    QList<MenuInfo> mMenuList;
    int hMenuNext;

    void initMenuApi();

protected:
    void dragEnterEvent(QDragEnterEvent* pEvent);
    void dropEvent(QDropEvent* pEvent);

};

#endif // MAINWINDOW_H
