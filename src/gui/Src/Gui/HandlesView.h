#ifndef HANDLESVIEW_H
#define HANDLESVIEW_H

#include <QWidget>
#include "Imports.h"

class StdTable;
class ReferenceView;
class QVBoxLayout;
class LabeledSplitter;
class HandlesWindowViewTable;

class HandlesView : public QWidget
{
    Q_OBJECT
public:
    explicit HandlesView(QWidget* parent = nullptr);

public slots:
    void reloadData();
    void refreshShortcuts();
    void dbgStateChanged(DBGSTATE state);

    void handlesTableContextMenuSlot(const QPoint & pos);
    void tcpConnectionsTableContextMenuSlot(const QPoint & pos);
    void windowsTableContextMenuSlot(const QPoint & pos);
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
    StdTable* mHandlesTable;
    StdTable* mTcpConnectionsTable;
    StdTable* mWindowsTable;
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

    void AppendPrivilege(int row, const char* PrivilegeString);
};

#endif // HANDLESVIEW_H
