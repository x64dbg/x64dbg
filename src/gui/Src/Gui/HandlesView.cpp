#include "HandlesView.h"
#include "Bridge.h"
#include "VersionHelpers.h"
#include "StdTable.h"
#include "LabeledSplitter.h"
#include <QVBoxLayout>

HandlesView::HandlesView(QWidget* parent) : QWidget(parent)
{
    mHandlesTable = new StdTable(this);
    mHandlesTable->setWindowTitle("Handles");
    mHandlesTable->setDrawDebugOnly(true);
    int wCharWidth = mHandlesTable->getCharWidth();
    mHandlesTable->setContextMenuPolicy(Qt::CustomContextMenu);
    mHandlesTable->addColumnAt(8 + 16 * wCharWidth, tr("Type"), false);
    mHandlesTable->addColumnAt(8 + 8 * wCharWidth, tr("Type number"), false);
    mHandlesTable->addColumnAt(8 + sizeof(duint) * 2 * wCharWidth, tr("Handle"), false);
    mHandlesTable->addColumnAt(8 + 16 * wCharWidth, tr("Access"), false);
    mHandlesTable->addColumnAt(8 + wCharWidth * 20, tr("Name"), false);
    mHandlesTable->loadColumnFromConfig("Handle");

    mTcpConnectionsTable = new StdTable(this);
    mTcpConnectionsTable->setWindowTitle("TcpConnections");
    mTcpConnectionsTable->setDrawDebugOnly(true);
    mTcpConnectionsTable->setContextMenuPolicy(Qt::CustomContextMenu);
    mTcpConnectionsTable->addColumnAt(8 + 64 * wCharWidth, tr("Remote address"), false);
    mTcpConnectionsTable->addColumnAt(8 + 64 * wCharWidth, tr("Local address"), false);
    mTcpConnectionsTable->addColumnAt(8 + 8 * wCharWidth, tr("State"), false);
    mTcpConnectionsTable->loadColumnFromConfig("TcpConnection");

    mPrivilegesTable = new StdTable(this);
    mPrivilegesTable->setWindowTitle("Privileges");
    mPrivilegesTable->setDrawDebugOnly(true);
    mPrivilegesTable->setContextMenuPolicy(Qt::CustomContextMenu);
    mPrivilegesTable->addColumnAt(8 + 32 * wCharWidth, tr("Privilege"), false);
    mPrivilegesTable->addColumnAt(8 + 16 * wCharWidth, tr("State"), false);
    mPrivilegesTable->loadColumnFromConfig("Privilege");

    // Splitter
    mSplitter = new LabeledSplitter(this);
    mSplitter->addWidget(mHandlesTable, tr("Handles"));
    mSplitter->addWidget(mTcpConnectionsTable, tr("TCP Connections"));
    mSplitter->addWidget(mPrivilegesTable, tr("Privileges"));
    mSplitter->collapseLowerTabs();

    // Layout
    mVertLayout = new QVBoxLayout;
    mVertLayout->setSpacing(0);
    mVertLayout->setContentsMargins(0, 0, 0, 0);
    mVertLayout->addWidget(mSplitter);
    this->setLayout(mVertLayout);
    mSplitter->loadFromConfig("HandlesViewSplitter");

    // Create the action list for the right click context menu
    mActionRefresh = new QAction(DIcon("arrow-restart.png"), tr("&Refresh"), this);
    connect(mActionRefresh, SIGNAL(triggered()), this, SLOT(reloadData()));
    addAction(mActionRefresh);
    mActionCloseHandle = new QAction(DIcon("disable.png"), tr("Close handle"), this);
    connect(mActionCloseHandle, SIGNAL(triggered()), this, SLOT(closeHandleSlot()));
    mActionDisablePrivilege = new QAction(DIcon("disable.png"), tr("Disable Privilege: "), this);
    connect(mActionDisablePrivilege, SIGNAL(triggered()), this, SLOT(disablePrivilegeSlot()));
    mActionEnablePrivilege = new QAction(DIcon("enable.png"), tr("Enable Privilege: "), this);
    connect(mActionEnablePrivilege, SIGNAL(triggered()), this, SLOT(enablePrivilegeSlot()));
    mActionDisableAllPrivileges = new QAction(DIcon("disable.png"), tr("Disable all privileges"), this);
    connect(mActionDisableAllPrivileges, SIGNAL(triggered()), this, SLOT(disableAllPrivilegesSlot()));
    mActionEnableAllPrivileges = new QAction(DIcon("enable.png"), tr("Enable all privileges"), this);
    connect(mActionEnableAllPrivileges, SIGNAL(triggered()), this, SLOT(enableAllPrivilegesSlot()));

    connect(mHandlesTable, SIGNAL(contextMenuSignal(const QPoint &)), this, SLOT(handlesTableContextMenuSlot(const QPoint &)));
    connect(mTcpConnectionsTable, SIGNAL(contextMenuSignal(const QPoint &)), this, SLOT(tcpConnectionsTableContextMenuSlot(const QPoint &)));
    connect(mPrivilegesTable, SIGNAL(contextMenuSignal(const QPoint &)), this, SLOT(privilegesTableContextMenuSlot(const QPoint &)));
    connect(Config(), SIGNAL(shortcutsUpdated()), this, SLOT(refreshShortcuts()));
    connect(Bridge::getBridge(), SIGNAL(dbgStateChanged(DBGSTATE)), this, SLOT(dbgStateChanged(DBGSTATE)));

    if(!IsWindowsVistaOrGreater())
    {
        mTcpConnectionsTable->setRowCount(1);
        mTcpConnectionsTable->setCellContent(0, 0, tr("TCP Connection enumeration is only available on Windows Vista or greater."));
        mTcpConnectionsTable->reloadData();
    }
    reloadData();
    refreshShortcuts();
}

void HandlesView::reloadData()
{
    if(DbgIsDebugging())
    {
        enumHandles();
        enumTcpConnections();
        enumPrivileges();
    }
    else
    {
        mHandlesTable->setRowCount(0);
        mHandlesTable->reloadData();
        mTcpConnectionsTable->setRowCount(0);
        mTcpConnectionsTable->reloadData();
        mPrivilegesTable->setRowCount(0);
        mPrivilegesTable->reloadData();
    }
}

void HandlesView::refreshShortcuts()
{
    mActionRefresh->setShortcut(ConfigShortcut("ActionRefresh"));
}

void HandlesView::dbgStateChanged(DBGSTATE state)
{
    if(state == stopped)
        reloadData();
}

void HandlesView::handlesTableContextMenuSlot(const QPoint & pos)
{
    StdTable & table = *mHandlesTable;
    QMenu wMenu;
    wMenu.addAction(mActionRefresh);
    if(mHandlesTable->getRowCount() != 0)
        wMenu.addAction(mActionCloseHandle);
    QMenu wCopyMenu(tr("&Copy"), this);
    wCopyMenu.setIcon(DIcon("copy.png"));
    table.setupCopyMenu(&wCopyMenu);
    if(wCopyMenu.actions().length())
    {
        wMenu.addSeparator();
        wMenu.addMenu(&wCopyMenu);
    }
    wMenu.exec(table.mapToGlobal(pos));
}

void HandlesView::tcpConnectionsTableContextMenuSlot(const QPoint & pos)
{
    StdTable & table = *mTcpConnectionsTable;
    QMenu wMenu;
    wMenu.addAction(mActionRefresh);
    QMenu wCopyMenu(tr("&Copy"), this);
    wCopyMenu.setIcon(DIcon("copy.png"));
    table.setupCopyMenu(&wCopyMenu);
    if(wCopyMenu.actions().length())
    {
        wMenu.addSeparator();
        wMenu.addMenu(&wCopyMenu);
    }
    wMenu.exec(table.mapToGlobal(pos));
}


void HandlesView::privilegesTableContextMenuSlot(const QPoint & pos)
{
    StdTable & table = *mPrivilegesTable;
    QMenu wMenu;
    bool isValid = (mPrivilegesTable->getRowCount() != 0 && mPrivilegesTable->getCellContent(mPrivilegesTable->getInitialSelection(), 1) != tr("Unknown"));
    wMenu.addAction(mActionRefresh);
    if(isValid)
    {
        if(mPrivilegesTable->getCellContent(mPrivilegesTable->getInitialSelection(), 1) == tr("Enabled"))
        {
            mActionDisablePrivilege->setText(tr("Disable Privilege: ") + mPrivilegesTable->getCellContent(mPrivilegesTable->getInitialSelection(), 0));
            wMenu.addAction(mActionDisablePrivilege);
        }
        else
        {
            mActionEnablePrivilege->setText(tr("Enable Privilege: ") + mPrivilegesTable->getCellContent(mPrivilegesTable->getInitialSelection(), 0));
            wMenu.addAction(mActionEnablePrivilege);
        }
    }
    wMenu.addAction(mActionDisableAllPrivileges);
    wMenu.addAction(mActionEnableAllPrivileges);
    QMenu wCopyMenu(tr("&Copy"), this);
    wCopyMenu.setIcon(DIcon("copy.png"));
    table.setupCopyMenu(&wCopyMenu);
    if(wCopyMenu.actions().length())
    {
        wMenu.addSeparator();
        wMenu.addMenu(&wCopyMenu);
    }
    wMenu.exec(table.mapToGlobal(pos));
}

void HandlesView::closeHandleSlot()
{
    DbgCmdExec(QString("handleclose %1").arg(mHandlesTable->getCellContent(mHandlesTable->getInitialSelection(), 2)).toUtf8().constData());
}

void HandlesView::enablePrivilegeSlot()
{
    DbgCmdExec(QString("EnablePrivilege \"%1\"").arg(mPrivilegesTable->getCellContent(mPrivilegesTable->getInitialSelection(), 0)).toUtf8().constData());
    enumPrivileges();
}

void HandlesView::disablePrivilegeSlot()
{
    DbgCmdExec(QString("DisablePrivilege \"%1\"").arg(mPrivilegesTable->getCellContent(mPrivilegesTable->getInitialSelection(), 0)).toUtf8().constData());
    enumPrivileges();
}

void HandlesView::enableAllPrivilegesSlot()
{
    for(int i = 0; i < mPrivilegesTable->getRowCount(); i++)
        if(mPrivilegesTable->getCellContent(i, 1) != tr("Unknown"))
            DbgCmdExec(QString("EnablePrivilege \"%1\"").arg(mPrivilegesTable->getCellContent(i, 0)).toUtf8().constData());
    enumPrivileges();
}

void HandlesView::disableAllPrivilegesSlot()
{
    for(int i = 0; i < mPrivilegesTable->getRowCount(); i++)
        if(mPrivilegesTable->getCellContent(i, 1) != tr("Unknown"))
            DbgCmdExec(QString("DisablePrivilege \"%1\"").arg(mPrivilegesTable->getCellContent(i, 0)).toUtf8().constData());
    enumPrivileges();
}

void HandlesView::enumHandles()
{
    BridgeList<HANDLEINFO> handles;
    if(DbgFunctions()->EnumHandles(&handles))
    {
        auto count = handles.Count();
        mHandlesTable->setRowCount(count);
        for(auto i = 0; i < count; i++)
        {
            const HANDLEINFO & handle = handles[i];
            char name[MAX_STRING_SIZE] = "";
            char typeName[MAX_STRING_SIZE] = "";
            DbgFunctions()->GetHandleName(handle.Handle, name, sizeof(name), typeName, sizeof(typeName));
            mHandlesTable->setCellContent(i, 0, typeName);
            mHandlesTable->setCellContent(i, 1, ToHexString(handle.TypeNumber));
            mHandlesTable->setCellContent(i, 2, ToHexString(handle.Handle));
            mHandlesTable->setCellContent(i, 3, ToHexString(handle.GrantedAccess));
            mHandlesTable->setCellContent(i, 4, name);
        }
    }
    else
        mHandlesTable->setRowCount(0);
    mHandlesTable->reloadData();
}

void HandlesView::enumPrivileges()
{
    mPrivilegesTable->setRowCount(35);
    AppendPrivilege(0, "SeAssignPrimaryTokenPrivilege");
    AppendPrivilege(1, "SeAuditPrivilege");
    AppendPrivilege(2, "SeBackupPrivilege");
    AppendPrivilege(3, "SeChangeNotifyPrivilege");
    AppendPrivilege(4, "SeCreateGlobalPrivilege");
    AppendPrivilege(5, "SeCreatePagefilePrivilege");
    AppendPrivilege(6, "SeCreatePermanentPrivilege");
    AppendPrivilege(7, "SeCreateSymbolicLinkPrivilege");
    AppendPrivilege(8, "SeCreateTokenPrivilege");
    AppendPrivilege(9, "SeDebugPrivilege");
    AppendPrivilege(10, "SeEnableDelegationPrivilege");
    AppendPrivilege(11, "SeImpersonatePrivilege");
    AppendPrivilege(12, "SeIncreaseBasePriorityPrivilege");
    AppendPrivilege(13, "SeIncreaseQuotaPrivilege");
    AppendPrivilege(14, "SeIncreaseWorkingSetPrivilege");
    AppendPrivilege(15, "SeLoadDriverPrivilege");
    AppendPrivilege(16, "SeLockMemoryPrivilege");
    AppendPrivilege(17, "SeMachineAccountPrivilege");
    AppendPrivilege(18, "SeManageVolumePrivilege");
    AppendPrivilege(19, "SeProfileSingleProcessPrivilege");
    AppendPrivilege(20, "SeRelabelPrivilege");
    AppendPrivilege(21, "SeRemoteShutdownPrivilege");
    AppendPrivilege(22, "SeRestorePrivilege");
    AppendPrivilege(23, "SeSecurityPrivilege");
    AppendPrivilege(24, "SeShutdownPrivilege");
    AppendPrivilege(25, "SeSyncAgentPrivilege");
    AppendPrivilege(26, "SeSystemEnvironmentPrivilege");
    AppendPrivilege(27, "SeSystemProfilePrivilege");
    AppendPrivilege(28, "SeSystemtimePrivilege");
    AppendPrivilege(29, "SeTakeOwnershipPrivilege");
    AppendPrivilege(30, "SeTcbPrivilege");
    AppendPrivilege(31, "SeTimeZonePrivilege");
    AppendPrivilege(32, "SeTrustedCredManAccessPrivilege");
    AppendPrivilege(33, "SeUndockPrivilege");
    AppendPrivilege(34, "SeUnsolicitedInputPrivilege");
    mPrivilegesTable->reloadData();
}

void HandlesView::AppendPrivilege(int row, const char* PrivilegeString)
{
    DbgCmdExecDirect(QString("GetPrivilegeState \"%1\"").arg(PrivilegeString).toUtf8().constData());
    mPrivilegesTable->setCellContent(row, 0, QString(PrivilegeString));
    switch(DbgValFromString("$result"))
    {
    default:
        mPrivilegesTable->setCellContent(row, 1, tr("Unknown"));
        break;
    case 1:
        mPrivilegesTable->setCellContent(row, 1, tr("Disabled"));
        break;
    case 2:
    case 3:
        mPrivilegesTable->setCellContent(row, 1, tr("Enabled"));
        break;
    }
}

void HandlesView::enumTcpConnections()
{
    BridgeList<TCPCONNECTIONINFO> connections;
    if(DbgFunctions()->EnumTcpConnections(&connections))
    {
        auto count = connections.Count();
        mTcpConnectionsTable->setRowCount(count);
        for(auto i = 0; i < count; i++)
        {
            const TCPCONNECTIONINFO & connection = connections[i];
            auto remoteText = QString("%1:%2").arg(connection.RemoteAddress).arg(connection.RemotePort);
            mTcpConnectionsTable->setCellContent(i, 0, remoteText);
            auto localText = QString("%1:%2").arg(connection.LocalAddress).arg(connection.LocalPort);
            mTcpConnectionsTable->setCellContent(i, 1, localText);
            mTcpConnectionsTable->setCellContent(i, 2, connection.StateText);
        }
    }
    else
        mTcpConnectionsTable->setRowCount(0);
    mTcpConnectionsTable->reloadData();
}
