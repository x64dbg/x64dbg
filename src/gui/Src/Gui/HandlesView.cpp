#include <Windows.h>
#include <QtWin>
#include "HandlesView.h"
#include "Bridge.h"
#include "VersionHelpers.h"
#include "StdTable.h"
#include "LabeledSplitter.h"
#include "StringUtil.h"
#include "ReferenceView.h"
#include "StdIconSearchListView.h"
#include "MainWindow.h"
#include "MessagesBreakpoints.h"
#include <QVBoxLayout>

HandlesView::HandlesView(QWidget* parent) : QWidget(parent)
{
    // Setup handles list
    mHandlesTable = new StdSearchListView(this, true, true);
    mHandlesTable->setInternalTitle("Handles");
    mHandlesTable->mSearchStartCol = 0;
    mHandlesTable->setDrawDebugOnly(true);
    mHandlesTable->setDisassemblyPopupEnabled(false);
    int wCharWidth = mHandlesTable->getCharWidth();
    mHandlesTable->addColumnAt(8 + 16 * wCharWidth, tr("Type"), true);
    mHandlesTable->addColumnAt(8 + 8 * wCharWidth, tr("Type number"), true, "", StdTable::SortBy::AsHex);
    mHandlesTable->addColumnAt(8 + sizeof(duint) * 2 * wCharWidth, tr("Handle"), true, "", StdTable::SortBy::AsHex);
    mHandlesTable->addColumnAt(8 + 16 * wCharWidth, tr("Access"), true, "", StdTable::SortBy::AsHex);
    mHandlesTable->addColumnAt(8 + wCharWidth * 20, tr("Name"), true);
    mHandlesTable->loadColumnFromConfig("Handle");

    // Setup windows list
    mWindowsTable = new StdIconSearchListView(this, true, true);
    mWindowsTable->setInternalTitle("Windows");
    mWindowsTable->setSearchStartCol(0);
    mWindowsTable->setDrawDebugOnly(true);
    wCharWidth = mWindowsTable->getCharWidth();
    mWindowsTable->addColumnAt(8 + sizeof(duint) * 2 * wCharWidth, tr("Proc"), true, "", StdTable::SortBy::AsHex);
    mWindowsTable->addColumnAt(8 + 8 * wCharWidth, tr("Handle"), true, "", StdTable::SortBy::AsHex);
    mWindowsTable->addColumnAt(8 + 120 * wCharWidth, tr("Title"), true);
    mWindowsTable->addColumnAt(8 + 40 * wCharWidth, tr("Class"), true);
    mWindowsTable->addColumnAt(8 + 8 * wCharWidth, tr("Thread"), true, "", StdTable::SortBy::AsHex);
    mWindowsTable->addColumnAt(8 + 16 * wCharWidth, tr("Style"), true, "", StdTable::SortBy::AsHex);
    mWindowsTable->addColumnAt(8 + 16 * wCharWidth, tr("StyleEx"), true, "", StdTable::SortBy::AsHex);
    mWindowsTable->addColumnAt(8 + 8 * wCharWidth, tr("Parent"), true);
    mWindowsTable->addColumnAt(8 + 20 * wCharWidth, tr("Size"), true);
    mWindowsTable->addColumnAt(8 + 6 * wCharWidth, tr("Enable"), true);
    mWindowsTable->loadColumnFromConfig("Window");
    mWindowsTable->setIconColumn(2);

    // Setup tcp list
    mTcpConnectionsTable = new StdSearchListView(this, true, true);
    mTcpConnectionsTable->setInternalTitle("TcpConnections");
    mTcpConnectionsTable->setSearchStartCol(0);
    mTcpConnectionsTable->setDrawDebugOnly(true);
    mTcpConnectionsTable->setDisassemblyPopupEnabled(false);
    wCharWidth = mTcpConnectionsTable->getCharWidth();
    mTcpConnectionsTable->addColumnAt(8 + 64 * wCharWidth, tr("Remote address"), true);
    mTcpConnectionsTable->addColumnAt(8 + 64 * wCharWidth, tr("Local address"), true);
    mTcpConnectionsTable->addColumnAt(8 + 8 * wCharWidth, tr("State"), true);
    mTcpConnectionsTable->loadColumnFromConfig("TcpConnection");

    /*
        mHeapsTable = new ReferenceView(this);
        mHeapsTable->setWindowTitle("Heaps");
        //mHeapsTable->setContextMenuPolicy(Qt::CustomContextMenu);
        mHeapsTable->addColumnAt(sizeof(duint) * 2, tr("Address"));
        mHeapsTable->addColumnAt(sizeof(duint) * 2, tr("Size"));
        mHeapsTable->addColumnAt(20, tr("Flags"));
        mHeapsTable->addColumnAt(50, tr("Comments"));
    */

    mPrivilegesTable = new StdTable(this);
    mPrivilegesTable->setWindowTitle("Privileges");
    mPrivilegesTable->setDrawDebugOnly(true);
    mPrivilegesTable->setDisassemblyPopupEnabled(false);
    mPrivilegesTable->setContextMenuPolicy(Qt::CustomContextMenu);
    mPrivilegesTable->addColumnAt(8 + 32 * wCharWidth, tr("Privilege"), true);
    mPrivilegesTable->addColumnAt(8 + 16 * wCharWidth, tr("State"), true);
    mPrivilegesTable->loadColumnFromConfig("Privilege");

    // Splitter
    mSplitter = new LabeledSplitter(this);
    mSplitter->addWidget(mWindowsTable, tr("Windows"));
    mSplitter->addWidget(mHandlesTable, tr("Handles"));
    //mSplitter->addWidget(mHeapsTable, tr("Heaps"));
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
    mActionEnableWindow = new QAction(DIcon("enable.png"), tr("Enable window"), this);
    connect(mActionEnableWindow, SIGNAL(triggered()), this, SLOT(enableWindowSlot()));
    mActionDisableWindow = new QAction(DIcon("disable.png"), tr("Disable window"), this);
    connect(mActionDisableWindow, SIGNAL(triggered()), this, SLOT(disableWindowSlot()));
    mActionFollowProc = new QAction(DIcon(ArchValue("processor32.png", "processor64.png")), tr("Follow Proc in Disassembler"), this);
    connect(mActionFollowProc, SIGNAL(triggered()), this, SLOT(followInDisasmSlot()));
    mActionFollowProc->setShortcut(Qt::Key_Return);
    mWindowsTable->addAction(mActionFollowProc);
    mActionToggleProcBP = new QAction(DIcon("breakpoint_toggle.png"), tr("Toggle Breakpoint in Proc"), this);
    connect(mActionToggleProcBP, SIGNAL(triggered()), this, SLOT(toggleBPSlot()));
    mActionMessageProcBP = new QAction(DIcon("breakpoint_execute.png"), tr("Message Breakpoint"), this);
    connect(mActionMessageProcBP, SIGNAL(triggered()), this, SLOT(messagesBPSlot()));

    connect(mHandlesTable, SIGNAL(listContextMenuSignal(QMenu*)), this, SLOT(handlesTableContextMenuSlot(QMenu*)));
    connect(mWindowsTable, SIGNAL(listContextMenuSignal(QMenu*)), this, SLOT(windowsTableContextMenuSlot(QMenu*)));
    connect(mTcpConnectionsTable, SIGNAL(listContextMenuSignal(QMenu*)), this, SLOT(tcpConnectionsTableContextMenuSlot(QMenu*)));
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
        enumWindows();
        enumTcpConnections();
        //enumHeaps();
        enumPrivileges();
    }
    else
    {
        mHandlesTable->setRowCount(0);
        mHandlesTable->reloadData();
        mWindowsTable->setRowCount(0);
        mWindowsTable->reloadData();
        mTcpConnectionsTable->setRowCount(0);
        mTcpConnectionsTable->reloadData();

        //mHeapsTable->setRowCount(0);
        //mHeapsTable->reloadData();
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

void HandlesView::handlesTableContextMenuSlot(QMenu* wMenu)
{
    if(!DbgIsDebugging())
        return;
    auto & table = *mHandlesTable->mCurList;
    QMenu wCopyMenu(tr("&Copy"), this);
    wCopyMenu.setIcon(DIcon("copy.png"));

    wMenu->addAction(mActionRefresh);
    if(table.getRowCount())
    {
        wMenu->addAction(mActionCloseHandle);

        table.setupCopyMenu(&wCopyMenu);
        if(wCopyMenu.actions().length())
        {
            wMenu->addSeparator();
            wMenu->addMenu(&wCopyMenu);
        }
    }
}

void HandlesView::windowsTableContextMenuSlot(QMenu* wMenu)
{
    if(!DbgIsDebugging())
        return;
    auto & table = *mWindowsTable->mCurList;
    QMenu wCopyMenu(tr("Copy"), this);
    wCopyMenu.setIcon(DIcon("copy.png"));
    wMenu->addAction(mActionRefresh);

    if(table.getRowCount())
    {
        if(table.getCellContent(table.getInitialSelection(), 9) == tr("Enabled"))
        {
            mActionDisableWindow->setText(tr("Disable window"));
            wMenu->addAction(mActionDisableWindow);
        }
        else
        {
            mActionEnableWindow->setText(tr("Enable window"));
            wMenu->addAction(mActionEnableWindow);
        }

        wMenu->addAction(mActionFollowProc);
        wMenu->addAction(mActionToggleProcBP);
        wMenu->addAction(mActionMessageProcBP);
        wMenu->addSeparator();
        table.setupCopyMenu(&wCopyMenu);
        if(wCopyMenu.actions().length())
        {
            wMenu->addSeparator();
            wMenu->addMenu(&wCopyMenu);
        }
    }
}

void HandlesView::tcpConnectionsTableContextMenuSlot(QMenu* wMenu)
{
    if(!DbgIsDebugging())
        return;
    auto & table = *mTcpConnectionsTable->mCurList;
    QMenu wCopyMenu(tr("&Copy"), this);
    wCopyMenu.setIcon(DIcon("copy.png"));

    wMenu->addAction(mActionRefresh);
    if(table.getRowCount())
    {
        table.setupCopyMenu(&wCopyMenu);
        if(wCopyMenu.actions().length())
        {
            wMenu->addSeparator();
            wMenu->addMenu(&wCopyMenu);
        }
    }
}

void HandlesView::privilegesTableContextMenuSlot(const QPoint & pos)
{
    if(!DbgIsDebugging())
        return;
    StdTable & table = *mPrivilegesTable;
    QMenu wMenu;
    bool isValid = (table.getRowCount() != 0 && table.getCellContent(table.getInitialSelection(), 1) != tr("Unknown"));
    wMenu.addAction(mActionRefresh);
    if(isValid)
    {
        if(table.getCellContent(table.getInitialSelection(), 1) == tr("Enabled"))
        {
            mActionDisablePrivilege->setText(tr("Disable Privilege: ") + table.getCellContent(table.getInitialSelection(), 0));
            wMenu.addAction(mActionDisablePrivilege);
        }
        else
        {
            mActionEnablePrivilege->setText(tr("Enable Privilege: ") + table.getCellContent(table.getInitialSelection(), 0));
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
    DbgCmdExecDirect(QString("handleclose %1").arg(mHandlesTable->mCurList->getCellContent(mHandlesTable->mCurList->getInitialSelection(), 2)));
    enumHandles();
}

void HandlesView::enablePrivilegeSlot()
{
    DbgCmdExecDirect(QString("EnablePrivilege \"%1\"").arg(mPrivilegesTable->getCellContent(mPrivilegesTable->getInitialSelection(), 0)));
    enumPrivileges();
}

void HandlesView::disablePrivilegeSlot()
{
    if(!DbgIsDebugging())
        return;
    DbgCmdExecDirect(QString("DisablePrivilege \"%1\"").arg(mPrivilegesTable->getCellContent(mPrivilegesTable->getInitialSelection(), 0)));
    enumPrivileges();
}

void HandlesView::enableAllPrivilegesSlot()
{
    if(!DbgIsDebugging())
        return;
    for(int i = 0; i < mPrivilegesTable->getRowCount(); i++)
        if(mPrivilegesTable->getCellContent(i, 1) != tr("Unknown"))
            DbgCmdExecDirect(QString("EnablePrivilege \"%1\"").arg(mPrivilegesTable->getCellContent(i, 0)));
    enumPrivileges();
}

void HandlesView::disableAllPrivilegesSlot()
{
    if(!DbgIsDebugging())
        return;
    for(int i = 0; i < mPrivilegesTable->getRowCount(); i++)
        if(mPrivilegesTable->getCellContent(i, 1) != tr("Unknown"))
            DbgCmdExecDirect(QString("DisablePrivilege \"%1\"").arg(mPrivilegesTable->getCellContent(i, 0)));
    enumPrivileges();
}

void HandlesView::enableWindowSlot()
{
    DbgCmdExecDirect(QString("EnableWindow %1").arg(mWindowsTable->mCurList->getCellContent(mWindowsTable->mCurList->getInitialSelection(), 1)));
    enumWindows();
}

void HandlesView::disableWindowSlot()
{
    DbgCmdExecDirect(QString("DisableWindow %1").arg(mWindowsTable->mCurList->getCellContent(mWindowsTable->mCurList->getInitialSelection(), 1)));
    enumWindows();
}

void HandlesView::followInDisasmSlot()
{
    DbgCmdExec(QString("disasm %1").arg(mWindowsTable->mCurList->getCellContent(mWindowsTable->mCurList->getInitialSelection(), 0)));
}

void HandlesView::toggleBPSlot()
{
    auto & mCurList = *mWindowsTable->mCurList;

    if(!DbgIsDebugging())
        return;

    if(!mCurList.getRowCount())
        return;
    QString addrText = mCurList.getCellContent(mCurList.getInitialSelection(), 0).toUtf8().constData();
    duint wVA;
    if(!DbgFunctions()->ValFromString(addrText.toUtf8().constData(), &wVA))
        return;
    if(!DbgMemIsValidReadPtr(wVA))
        return;

    BPXTYPE wBpType = DbgGetBpxTypeAt(wVA);
    QString wCmd;

    if((wBpType & bp_normal) == bp_normal)
        wCmd = "bc " + ToPtrString(wVA);
    else if(wBpType == bp_none)
        wCmd = "bp " + ToPtrString(wVA);

    DbgCmdExecDirect(wCmd);
}

void HandlesView::messagesBPSlot()
{
    auto & mCurList = *mWindowsTable->mCurList;
    MessagesBreakpoints::MsgBreakpointData mbpData;

    if(!mCurList.getRowCount())
        return;

    mbpData.wndHandle = mCurList.getCellContent(mCurList.getInitialSelection(), 1).toUtf8().constData();
    mbpData.procVA = mCurList.getCellContent(mCurList.getInitialSelection(), 0).toUtf8().constData();

    MessagesBreakpoints messagesBPDialog(mbpData, this);
    messagesBPDialog.exec();
}

//Enum functions
//Enumerate handles and update handles table
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
    // refresh values also when in mSearchList
    mHandlesTable->refreshSearchList();
}

static QIcon getWindowIcon(HWND hWnd)
{
    HICON winIcon;
    if(IsWindowUnicode(hWnd))
    {
        //Some windows only return an icon via WM_GETICON, but SendMessage is generally unsafe
        //if(SendMessageTimeoutW(hWnd, WM_GETICON, 0, 0, SMTO_ABORTIFHUNG | SMTO_BLOCK | SMTO_ERRORONEXIT, 500, (PDWORD)&winIcon) == 0)
        winIcon = (HICON)GetClassLongPtrW(hWnd, -14); //GCL_HICON
    }
    else
    {
        //if(SendMessageTimeoutA(hWnd, WM_GETICON, 0, 0, SMTO_ABORTIFHUNG | SMTO_BLOCK | SMTO_ERRORONEXIT, 500, (PDWORD)&winIcon) == 0)
        winIcon = (HICON)GetClassLongPtrA(hWnd, -14); //GCL_HICON
    }
    QIcon result;
    if(winIcon != 0)
    {
        result = QIcon(QtWin::fromHICON(winIcon));
        DestroyIcon(winIcon);
    }
    return result;
}

//Enumerate windows and update windows table
void HandlesView::enumWindows()
{
    BridgeList<WINDOW_INFO> windows;
    if(DbgFunctions()->EnumWindows(&windows))
    {
        auto count = windows.Count();
        mWindowsTable->setRowCount(count);
        for(auto i = 0; i < count; i++)
        {
            mWindowsTable->setCellContent(i, 0, ToPtrString(windows[i].wndProc));
            mWindowsTable->setCellContent(i, 1, ToHexString(windows[i].handle));
            mWindowsTable->setCellContent(i, 2, QString(windows[i].windowTitle));
            mWindowsTable->setCellContent(i, 3, QString(windows[i].windowClass));
            char threadname[MAX_THREAD_NAME_SIZE];
            if(DbgFunctions()->ThreadGetName(windows[i].threadId, threadname) && *threadname != '\0')
                mWindowsTable->setCellContent(i, 4, QString::fromUtf8(threadname));
            else if(Config()->getBool("Gui", "PidTidInHex"))
                mWindowsTable->setCellContent(i, 4, ToHexString(windows[i].threadId));
            else
                mWindowsTable->setCellContent(i, 4, QString::number(windows[i].threadId));
            //Style
            mWindowsTable->setCellContent(i, 5, ToHexString(windows[i].style));
            //StyleEx
            mWindowsTable->setCellContent(i, 6, ToHexString(windows[i].styleEx));
            mWindowsTable->setCellContent(i, 7, ToHexString(windows[i].parent) + (windows[i].parent == ((duint)GetDesktopWindow()) ? tr(" (Desktop window)") : ""));
            //Size
            QString sizeText = QString("(%1,%2);%3x%4").arg(windows[i].position.left).arg(windows[i].position.top)
                               .arg(windows[i].position.right - windows[i].position.left).arg(windows[i].position.bottom - windows[i].position.top);
            mWindowsTable->setCellContent(i, 8, sizeText);
            mWindowsTable->setCellContent(i, 9, windows[i].enabled != FALSE ? tr("Enabled") : tr("Disabled"));
            mWindowsTable->setRowIcon(i, getWindowIcon((HWND)windows[i].handle));
        }
    }
    else
        mWindowsTable->setRowCount(0);
    mWindowsTable->reloadData();
    // refresh values also when in mSearchList
    mWindowsTable->refreshSearchList();
}

//Enumerate privileges and update privileges table
void HandlesView::enumPrivileges()
{
    const char* PrivilegeString[] = {"SeAssignPrimaryTokenPrivilege", "SeAuditPrivilege", "SeBackupPrivilege",
                                     "SeChangeNotifyPrivilege", "SeCreateGlobalPrivilege", "SeCreatePagefilePrivilege",
                                     "SeCreatePermanentPrivilege", "SeCreateSymbolicLinkPrivilege", "SeCreateTokenPrivilege",
                                     "SeDebugPrivilege", "SeEnableDelegationPrivilege", "SeImpersonatePrivilege",
                                     "SeIncreaseBasePriorityPrivilege", "SeIncreaseQuotaPrivilege", "SeIncreaseWorkingSetPrivilege",
                                     "SeLoadDriverPrivilege", "SeLockMemoryPrivilege", "SeMachineAccountPrivilege",
                                     "SeManageVolumePrivilege", "SeProfileSingleProcessPrivilege", "SeRelabelPrivilege",
                                     "SeRemoteShutdownPrivilege", "SeRestorePrivilege", "SeSecurityPrivilege",
                                     "SeShutdownPrivilege", "SeSyncAgentPrivilege", "SeSystemEnvironmentPrivilege",
                                     "SeSystemProfilePrivilege", "SeSystemtimePrivilege", "SeTakeOwnershipPrivilege",
                                     "SeTcbPrivilege", "SeTimeZonePrivilege", "SeTrustedCredManAccessPrivilege",
                                     "SeUndockPrivilege", "SeUnsolicitedInputPrivilege"
                                    };
    mPrivilegesTable->setRowCount(_countof(PrivilegeString));
    for(size_t row = 0; row < _countof(PrivilegeString); row++)
    {
        QString temp(PrivilegeString[row]);
        DbgCmdExecDirect(QString("GetPrivilegeState \"%1\"").arg(temp).toUtf8().constData());
        mPrivilegesTable->setCellContent(row, 0, temp);
        switch(DbgValFromString("$result"))
        {
        default:
            temp = tr("Unknown");
            break;
        case 1:
            temp = tr("Disabled");
            break;
        case 2:
        case 3:
            temp = tr("Enabled");
            break;
        }
        mPrivilegesTable->setCellContent(row, 1, temp);
    }

    mPrivilegesTable->reloadData();
}

//Enumerate TCP connections and update TCP connections table
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
    // refresh values also when in mSearchList
    mTcpConnectionsTable->refreshSearchList();
}

/*
//Enumerate Heaps and update Heaps table
void HandlesView::enumHeaps()
{
    BridgeList<HEAPINFO> heaps;
    if(DbgFunctions()->EnumHeaps(&heaps))
    {
        auto count = heaps.Count();
        mHeapsTable->setRowCount(count);
        for(auto i = 0; i < count; i++)
        {
            const HEAPINFO & heap = heaps[i];
            mHeapsTable->setCellContent(i, 0, ToPtrString(heap.addr));
            mHeapsTable->setCellContent(i, 1, ToHexString(heap.size));
            QString flagsText;
            if(heap.flags == 0)
                flagsText = " |"; //Always leave 2 characters to be removed
            if(heap.flags & 1)
                flagsText = "LF32_FIXED |";
            if(heap.flags & 2)
                flagsText += "LF32_FREE |";
            if(heap.flags & 4)
                flagsText += "LF32_MOVABLE |";
            if(heap.flags & (~7))
                flagsText += ToHexString(heap.flags & (~7)) + " |";
            flagsText.chop(2); //Remove last 2 characters: " |"
            mHeapsTable->setCellContent(i, 2, flagsText);
            QString comment;
            char commentUtf8[MAX_COMMENT_SIZE];
            if(DbgGetCommentAt(heap.addr, commentUtf8))
                comment = QString::fromUtf8(commentUtf8);
            else
            {
                if(DbgGetStringAt(heap.addr, commentUtf8))
                    comment = QString::fromUtf8(commentUtf8);
            }
            mHeapsTable->setCellContent(i, 3, comment);
        }
    }
    else
        mHeapsTable->setRowCount(0);
    mHeapsTable->reloadData();
}
*/
