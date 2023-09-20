#pragma once

#include <QWidget>
#include "Imports.h"

class StdTable;
class ReferenceView;
class QVBoxLayout;
class LabeledSplitter;
class StdSearchListView;
class StdIconSearchListView;
class QMenu;

class HandlesView : public QWidget
{
    Q_OBJECT
public:
    explicit HandlesView(QWidget* parent = nullptr);

public slots:
    void reloadData();
    void refreshShortcuts();
    void dbgStateChanged(DBGSTATE state);

    void handlesTableContextMenuSlot(QMenu* menu);
    void tcpConnectionsTableContextMenuSlot(QMenu* menu);
    void windowsTableContextMenuSlot(QMenu*);
    void privilegesTableContextMenuSlot(const QPoint & pos);

    void closeHandleSlot();
    void disablePrivilegeSlot();
    void enablePrivilegeSlot();
    void disableAllPrivilegesSlot();
    void enableAllPrivilegesSlot();
    void enableWindowSlot();
    void disableWindowSlot();
    void followInDisasmSlot();
    void toggleBPSlot();
    void messagesBPSlot();

private:
    QVBoxLayout* mVertLayout;
    LabeledSplitter* mSplitter;
    StdSearchListView* mHandlesTable;
    StdSearchListView* mTcpConnectionsTable;
    StdIconSearchListView* mWindowsTable;
    //ReferenceView* mHeapsTable;
    StdTable* mPrivilegesTable;

    QAction* mActionRefresh;
    QAction* mActionCloseHandle;
    QAction* mActionDisablePrivilege;
    QAction* mActionEnablePrivilege;
    QAction* mActionDisableAllPrivileges;
    QAction* mActionEnableAllPrivileges;
    QAction* mActionEnableWindow;
    QAction* mActionDisableWindow;
    QAction* mActionFollowProc;
    QAction* mActionToggleProcBP;
    QAction* mActionMessageProcBP;

    void enumHandles();
    void enumWindows();
    void enumTcpConnections();
    //void enumHeaps();
    void enumPrivileges();
};
