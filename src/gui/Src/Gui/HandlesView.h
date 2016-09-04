#ifndef HANDLESVIEW_H
#define HANDLESVIEW_H

#include <QWidget>
#include "Imports.h"

class StdTable;
class QVBoxLayout;
class LabeledSplitter;

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
    void privilegesTableContextMenuSlot(const QPoint & pos);

    void closeHandleSlot();
    void disablePrivilegeSlot();
    void enablePrivilegeSlot();
    void disableAllPrivilegesSlot();
    void enableAllPrivilegesSlot();

private:
    QVBoxLayout* mVertLayout;
    LabeledSplitter* mSplitter;
    StdTable* mHandlesTable;
    StdTable* mTcpConnectionsTable;
    StdTable* mPrivilegesTable;

    QAction* mActionRefresh;
    QAction* mActionCloseHandle;
    QAction* mActionDisablePrivilege;
    QAction* mActionEnablePrivilege;
    QAction* mActionDisableAllPrivileges;
    QAction* mActionEnableAllPrivileges;

    void enumHandles();
    void enumTcpConnections();
    void enumPrivileges();

    void AppendPrivilege(int row, const char* PrivilegeString);
};

#endif // HANDLESVIEW_H
