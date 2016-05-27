#include <WS2tcpip.h>
#define _NO_ADDRINFO
#include "HandlesView.h"
#include "Bridge.h"
#include "IPHlpApi.h"
#include "VersionHelpers.h"
#pragma comment(lib, "ws2_32.lib")

HandlesView::HandlesView(QWidget* parent) : QWidget(parent)
{
    mHandlesTable = new StdTable(this);
    int wCharWidth = mHandlesTable->getCharWidth();
    mHandlesTable->setContextMenuPolicy(Qt::CustomContextMenu);
    mHandlesTable->addColumnAt(8 + 16 * wCharWidth, tr("Type"), false);
    mHandlesTable->addColumnAt(8 + 8 * wCharWidth, tr("Type number"), false);
    mHandlesTable->addColumnAt(8 + sizeof(duint) * 2 * wCharWidth, tr("Handle"), false);
    mHandlesTable->addColumnAt(8 + 16 * wCharWidth, tr("Access"), false);
    mHandlesTable->addColumnAt(8 + wCharWidth * 20, tr("Name"), false);

    mTcpConnectionsTable = new StdTable(this);
    mTcpConnectionsTable->setContextMenuPolicy(Qt::CustomContextMenu);
    mTcpConnectionsTable->addColumnAt(8 + 64 * wCharWidth, tr("Remote address"), false);
    mTcpConnectionsTable->addColumnAt(8 + 64 * wCharWidth, tr("Local address"), false);
    mTcpConnectionsTable->addColumnAt(8 + 8 * wCharWidth, tr("State", "TcpConnection"), false);

    mPrivilegesTable = new StdTable(this);
    mPrivilegesTable->setContextMenuPolicy(Qt::CustomContextMenu);
    mPrivilegesTable->addColumnAt(8 + 32 * wCharWidth, tr("Privilege"), false);
    mPrivilegesTable->addColumnAt(8 + 16 * wCharWidth, tr("State", "Privilege"), false);

    // Splitter
    mSplitter = new QSplitter(this);
    mSplitter->setOrientation(Qt::Vertical);
    mSplitter->addWidget(mHandlesTable);
    mSplitter->addWidget(mTcpConnectionsTable);
    mSplitter->addWidget(mPrivilegesTable);

    // Layout
    mVertLayout = new QVBoxLayout;
    mVertLayout->setSpacing(0);
    mVertLayout->setContentsMargins(0, 0, 0, 0);
    mVertLayout->addWidget(mSplitter);
    this->setLayout(mVertLayout);

    // Create the action list for the right click context menu
    mActionRefresh = new QAction(QIcon(":/images/arrow_restart.png"), tr("&Refresh"), this);
    connect(mActionRefresh, SIGNAL(triggered()), this, SLOT(reloadData()));
    mActionCloseHandle = new QAction(QIcon(":/images/close-all-tabs.png"), tr("Close handle"), this);
    connect(mActionCloseHandle, SIGNAL(triggered()), this, SLOT(closeHandleSlot()));
    mActionDisablePrivilege = new QAction(QIcon(":/images/close-all-tabs.png"), tr("Disable Privilege: "), this);
    connect(mActionDisablePrivilege, SIGNAL(triggered()), this, SLOT(disablePrivilegeSlot()));
    mActionEnablePrivilege = new QAction(tr("Enable Privilege: "), this);
    connect(mActionEnablePrivilege, SIGNAL(triggered()), this, SLOT(enablePrivilegeSlot()));
    mActionDisableAllPrivileges = new QAction(QIcon(":/images/close-all-tabs.png"), tr("Disable all privileges"), this);
    connect(mActionDisableAllPrivileges, SIGNAL(triggered()), this, SLOT(disableAllPrivilegesSlot()));
    mActionEnableAllPrivileges = new QAction(tr("Enable all privileges"), this);
    connect(mActionEnableAllPrivileges, SIGNAL(triggered()), this, SLOT(enableAllPrivilegesSlot()));

    connect(mHandlesTable, SIGNAL(contextMenuSignal(const QPoint &)), this, SLOT(handlesTableContextMenuSlot(const QPoint &)));
    connect(mTcpConnectionsTable, SIGNAL(contextMenuSignal(const QPoint &)), this, SLOT(tcpConnectionsTableContextMenuSlot(const QPoint &)));
    connect(mPrivilegesTable, SIGNAL(contextMenuSignal(const QPoint &)), this, SLOT(privilegesTableContextMenuSlot(const QPoint &)));

    if(IsWindowsVistaOrGreater())
        hIpHlp = LoadLibraryW(L"iphlpapi.dll");
    else
    {
        hIpHlp = 0;
        mTcpConnectionsTable->setRowCount(1);
        mTcpConnectionsTable->setCellContent(0, 0, tr("TCP Connection enumeration is only available on Windows Vista or greater."));
        mTcpConnectionsTable->reloadData();
    }
    reloadData();
}

HandlesView::~HandlesView()
{
    if(hIpHlp)
        FreeLibrary(hIpHlp);
}

void HandlesView::reloadData()
{
    if(DbgIsDebugging())
    {
        enumHandles();
        enumTcpConnections();
        enumPrivileges();
    }
}

void HandlesView::handlesTableContextMenuSlot(const QPoint & pos)
{
    StdTable & table = *mHandlesTable;
    QMenu wMenu;
    wMenu.addAction(mActionRefresh);
    wMenu.addAction(mActionCloseHandle);
    QMenu wCopyMenu(tr("&Copy"));
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
    QMenu wCopyMenu(tr("&Copy"));
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
    bool isValid = (mPrivilegesTable->getCellContent(mPrivilegesTable->getInitialSelection(), 1) != tr("Unknown"));
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
    QMenu wCopyMenu(tr("&Copy"));
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
    duint remotehandle;
    if(!DbgIsDebugging())
        return;
    if(DbgFunctions()->ValFromString(mHandlesTable->getCellContent(mHandlesTable->getInitialSelection(), 2).toUtf8().constData(), &remotehandle))
    {
        HANDLE localHandle = nullptr;
        DuplicateHandle(DbgGetProcessInformation()->hProcess, (HANDLE)remotehandle, GetCurrentProcess(), &localHandle, DUPLICATE_SAME_ACCESS, FALSE, DUPLICATE_CLOSE_SOURCE);
        if(localHandle)
            CloseHandle(localHandle);
    }
    enumHandles();
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
    long handleCount = DbgGetHandleCount();
    if(handleCount > 0)
    {
        duint* allHandles = new duint[handleCount + 16];
        unsigned char* typeNumbers = new unsigned char[handleCount + 16];
        unsigned int* grantedAccess = new unsigned int[handleCount + 16];
        memset(allHandles, 0, sizeof(duint) * (handleCount + 16));
        memset(typeNumbers, 0, handleCount + 16);
        memset(grantedAccess, 0, sizeof(unsigned int) * (handleCount + 16));
        duint ret1 = DbgEnumHandles(allHandles, typeNumbers, grantedAccess, handleCount + 16);
        mHandlesTable->setRowCount(ret1);
        for(unsigned int i = 0; i < ret1; i++)
        {
            char name[512];
            char typeStr[512];
            memset(name, 0, sizeof(name));
            memset(typeStr, 0, sizeof(typeStr));
            DbgGetHandleName(name, typeStr, sizeof(name), allHandles[i]);
            mHandlesTable->setCellContent(i, 0, typeStr);
            mHandlesTable->setCellContent(i, 1, ToHexString(typeNumbers[i]));
            mHandlesTable->setCellContent(i, 2, ToHexString(allHandles[i]));
            mHandlesTable->setCellContent(i, 3, ToHexString(grantedAccess[i]));
            mHandlesTable->setCellContent(i, 4, name);
        }
        delete allHandles;
        delete typeNumbers;
        delete grantedAccess;
    }
    else
    {
        mHandlesTable->setRowCount(1);
        mHandlesTable->setCellContent(0, 0, QString("error:%1").arg(handleCount));
    }
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
    if(!hIpHlp)
        return;
    QList<QString> TCPLocal;
    QList<QString> TCPRemote;
    QList<QString> TCPState;
    DWORD PID = DbgGetProcessInformation()->dwProcessId;
    // The following code is modified from code sample at MSDN.GetTcpTable2
    // Declare and initialize variables
    PMIB_TCPTABLE2 pTcpTable;
    PMIB_TCP6TABLE2 pTcp6Table;
    ULONG ulSize = 0;
    struct in_addr IpAddr;
    int i;
    // To ensure WindowsXP compatibility we won't link them statically
    ULONG(WINAPI * GetTcpTable2)(PMIB_TCPTABLE2, PULONG, BOOL);
    *(FARPROC*)&GetTcpTable2 = GetProcAddress(hIpHlp, "GetTcpTable2");
    ULONG(WINAPI * GetTcp6Table2)(PMIB_TCP6TABLE2 TcpTable, PULONG, BOOL Order);
    *(FARPROC*)&GetTcp6Table2 = GetProcAddress(hIpHlp, "GetTcp6Table2");
    PCTSTR(WSAAPI * InetNtopW)(INT Family, PVOID  pAddr, PTSTR  pStringBuf, size_t StringBufSize);
    *(FARPROC*)&InetNtopW = GetProcAddress(GetModuleHandleW(L"ws2_32.dll"), "InetNtopW");
    if(InetNtopW == nullptr)
        return;
    pTcpTable = (MIB_TCPTABLE2*) malloc(sizeof(MIB_TCPTABLE2));
    ulSize = sizeof(MIB_TCPTABLE);
    // Make an initial call to GetTcpTable2 to
    // get the necessary size into the ulSize variable
    if(GetTcpTable2 != nullptr && GetTcpTable2(pTcpTable, &ulSize, TRUE) == ERROR_INSUFFICIENT_BUFFER)
    {
        free(pTcpTable);
        pTcpTable = (MIB_TCPTABLE2*) malloc(ulSize);
    }
    // Make a second call to GetTcpTable2 to get
    // the actual data we require
    if(GetTcpTable2 != nullptr && GetTcpTable2(pTcpTable, &ulSize, TRUE) == NO_ERROR)
    {
        for(i = 0; i < (int) pTcpTable->dwNumEntries; i++)
        {
            wchar_t Buffer[56];
            if(pTcpTable->table[i].dwOwningPid != PID)
                continue;
            TCPState.push_back(TcpStateToString(pTcpTable->table[i].dwState));
            IpAddr.S_un.S_addr = (u_long) pTcpTable->table[i].dwLocalAddr;
            InetNtopW(AF_INET, &IpAddr, Buffer, 56);
            TCPLocal.push_back(QString("%1:%2").arg(QString().fromUtf16(Buffer)).arg(ntohs((u_short)pTcpTable->table[i].dwLocalPort)));

            IpAddr.S_un.S_addr = (u_long) pTcpTable->table[i].dwRemoteAddr;
            InetNtopW(AF_INET, &IpAddr, Buffer, 56);
            TCPRemote.push_back(QString("%1:%2").arg(QString().fromUtf16(Buffer)).arg(ntohs((u_short)pTcpTable->table[i].dwRemotePort)));
        }
    }
    if(pTcpTable != NULL)
    {
        free(pTcpTable);
        pTcpTable = NULL;
    }
    pTcp6Table = (MIB_TCP6TABLE2*) malloc(sizeof(MIB_TCP6TABLE2));
    ulSize = sizeof(MIB_TCP6TABLE);
    // Make an initial call to GetTcpTable2 to
    // get the necessary size into the ulSize variable
    if(GetTcp6Table2 != nullptr && GetTcp6Table2(pTcp6Table, &ulSize, TRUE) == ERROR_INSUFFICIENT_BUFFER)
    {
        free(pTcp6Table);
        pTcp6Table = (MIB_TCP6TABLE2*) malloc(ulSize);
    }
    // Make a second call to GetTcpTable2 to get
    // the actual data we require
    if(GetTcp6Table2 != nullptr && GetTcp6Table2(pTcp6Table, &ulSize, TRUE) == NO_ERROR)
    {
        for(i = 0; i < (int) pTcp6Table->dwNumEntries; i++)
        {
            wchar_t Buffer[56];
            if(pTcp6Table->table[i].dwOwningPid != PID)
                continue;
            TCPState.push_back(TcpStateToString(pTcp6Table->table[i].State));
            InetNtopW(AF_INET6, &pTcp6Table->table[i].LocalAddr, Buffer, 56);
            TCPLocal.push_back(QString("[%1]:%2").arg(QString().fromUtf16(Buffer)).arg(ntohs((u_short)pTcp6Table->table[i].dwLocalPort)));
            InetNtopW(AF_INET6, &pTcp6Table->table[i].RemoteAddr, Buffer, 56);
            TCPRemote.push_back(QString("[%1]:%2").arg(QString().fromUtf16(Buffer)).arg(ntohs((u_short)pTcp6Table->table[i].dwRemotePort)));
        }
    }
    if(pTcp6Table != NULL)
    {
        free(pTcp6Table);
        pTcp6Table = NULL;
    }
    mTcpConnectionsTable->setRowCount(TCPRemote.length());
    for(int i = 0; i < TCPLocal.length(); i++)
    {
        mTcpConnectionsTable->setCellContent(i, 0, TCPRemote.at(i));
        mTcpConnectionsTable->setCellContent(i, 1, TCPLocal.at(i));
        mTcpConnectionsTable->setCellContent(i, 2, TCPState.at(i));
    }
    mTcpConnectionsTable->reloadData();
}

QString HandlesView::TcpStateToString(DWORD State)
{
    switch(State)
    {
    case MIB_TCP_STATE_CLOSED:
        return "CLOSED";
        break;
    case MIB_TCP_STATE_LISTEN:
        return "LISTEN";
        break;
    case MIB_TCP_STATE_SYN_SENT:
        return "SYN-SENT";
        break;
    case MIB_TCP_STATE_SYN_RCVD:
        return "SYN-RECEIVED";
        break;
    case MIB_TCP_STATE_ESTAB:
        return "ESTABLISHED";
        break;
    case MIB_TCP_STATE_FIN_WAIT1:
        return "FIN-WAIT-1";
        break;
    case MIB_TCP_STATE_FIN_WAIT2:
        return "FIN-WAIT-2";
        break;
    case MIB_TCP_STATE_CLOSE_WAIT:
        return "CLOSE-WAIT";
        break;
    case MIB_TCP_STATE_CLOSING:
        return "CLOSING";
        break;
    case MIB_TCP_STATE_LAST_ACK:
        return "LAST-ACK";
        break;
    case MIB_TCP_STATE_TIME_WAIT:
        return "TIME-WAIT";
        break;
    case MIB_TCP_STATE_DELETE_TCB:
        return "DELETE-TCB";
        break;
    default:
        return QString("UNKNOWN dwState value %1").arg(State);
        break;
    }
}
