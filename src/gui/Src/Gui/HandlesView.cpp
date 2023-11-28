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
#include "DisassemblyPopup.h"
#include <QVBoxLayout>

HandlesView::HandlesView(QWidget* parent) : QWidget(parent)
{
    // Setup handles list
    mHandlesTable = new StdSearchListView(this, true, true);
    mHandlesTable->setInternalTitle("Handles");
    mHandlesTable->mSearchStartCol = 0;
    mHandlesTable->setDrawDebugOnly(true);
    int charWidth = mHandlesTable->getCharWidth();
    mHandlesTable->addColumnAt(8 + 16 * charWidth, tr("Type"), true);
    mHandlesTable->addColumnAt(8 + 8 * charWidth, tr("Type number"), true, "", StdTable::SortBy::AsHex);
    mHandlesTable->addColumnAt(8 + sizeof(duint) * 2 * charWidth, tr("Handle"), true, "", StdTable::SortBy::AsHex);
    mHandlesTable->addColumnAt(8 + 16 * charWidth, tr("Access"), true, "", StdTable::SortBy::AsHex);
    mHandlesTable->addColumnAt(8 + charWidth * 20, tr("Name"), true);
    mHandlesTable->loadColumnFromConfig("Handle");

    // Add disassembly popups
    new DisassemblyPopup(mHandlesTable->stdList(), Bridge::getArchitecture());
    new DisassemblyPopup(mHandlesTable->stdSearchList(), Bridge::getArchitecture());

    // Setup windows list
    mWindowsTable = new StdIconSearchListView(this, true, true);
    mWindowsTable->setInternalTitle("Windows");
    mWindowsTable->setSearchStartCol(0);
    mWindowsTable->setDrawDebugOnly(true);
    charWidth = mWindowsTable->getCharWidth();
    mWindowsTable->addColumnAt(8 + sizeof(duint) * 2 * charWidth, tr("Proc"), true, "", StdTable::SortBy::AsHex);
    mWindowsTable->addColumnAt(8 + 8 * charWidth, tr("Handle"), true, "", StdTable::SortBy::AsHex);
    mWindowsTable->addColumnAt(8 + 120 * charWidth, tr("Title"), true);
    mWindowsTable->addColumnAt(8 + 40 * charWidth, tr("Class"), true);
    mWindowsTable->addColumnAt(8 + 8 * charWidth, tr("Thread"), true, "", StdTable::SortBy::AsHex);
    mWindowsTable->addColumnAt(8 + 16 * charWidth, tr("Style"), true, "", StdTable::SortBy::AsHex);
    mWindowsTable->addColumnAt(8 + 16 * charWidth, tr("StyleEx"), true, "", StdTable::SortBy::AsHex);
    mWindowsTable->addColumnAt(8 + 8 * charWidth, tr("Parent"), true);
    mWindowsTable->addColumnAt(8 + 20 * charWidth, tr("Size"), true);
    mWindowsTable->addColumnAt(8 + 6 * charWidth, tr("Enable"), true);
    mWindowsTable->loadColumnFromConfig("Window");
    mWindowsTable->setIconColumn(2);

    // Setup tcp list
    mTcpConnectionsTable = new StdSearchListView(this, true, true);
    mTcpConnectionsTable->setInternalTitle("TcpConnections");
    mTcpConnectionsTable->setSearchStartCol(0);
    mTcpConnectionsTable->setDrawDebugOnly(true);
    charWidth = mTcpConnectionsTable->getCharWidth();
    mTcpConnectionsTable->addColumnAt(8 + 64 * charWidth, tr("Remote address"), true);
    mTcpConnectionsTable->addColumnAt(8 + 64 * charWidth, tr("Local address"), true);
    mTcpConnectionsTable->addColumnAt(8 + 8 * charWidth, tr("State"), true);
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
    mPrivilegesTable->setContextMenuPolicy(Qt::CustomContextMenu);
    mPrivilegesTable->addColumnAt(8 + 32 * charWidth, tr("Privilege"), true);
    mPrivilegesTable->addColumnAt(8 + 16 * charWidth, tr("State"), true);
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
    mActionRefresh = new QAction(DIcon("arrow-restart"), tr("&Refresh"), this);
    connect(mActionRefresh, SIGNAL(triggered()), this, SLOT(reloadData()));
    addAction(mActionRefresh);
    mActionCloseHandle = new QAction(DIcon("disable"), tr("Close handle"), this);
    connect(mActionCloseHandle, SIGNAL(triggered()), this, SLOT(closeHandleSlot()));
    mActionDisablePrivilege = new QAction(DIcon("disable"), tr("Disable Privilege: "), this);
    connect(mActionDisablePrivilege, SIGNAL(triggered()), this, SLOT(disablePrivilegeSlot()));
    mActionEnablePrivilege = new QAction(DIcon("enable"), tr("Enable Privilege: "), this);
    connect(mActionEnablePrivilege, SIGNAL(triggered()), this, SLOT(enablePrivilegeSlot()));
    mActionDisableAllPrivileges = new QAction(DIcon("disable"), tr("Disable all privileges"), this);
    connect(mActionDisableAllPrivileges, SIGNAL(triggered()), this, SLOT(disableAllPrivilegesSlot()));
    mActionEnableAllPrivileges = new QAction(DIcon("enable"), tr("Enable all privileges"), this);
    connect(mActionEnableAllPrivileges, SIGNAL(triggered()), this, SLOT(enableAllPrivilegesSlot()));
    mActionEnableWindow = new QAction(DIcon("enable"), tr("Enable window"), this);
    connect(mActionEnableWindow, SIGNAL(triggered()), this, SLOT(enableWindowSlot()));
    mActionDisableWindow = new QAction(DIcon("disable"), tr("Disable window"), this);
    connect(mActionDisableWindow, SIGNAL(triggered()), this, SLOT(disableWindowSlot()));
    mActionFollowProc = new QAction(DIcon(ArchValue("processor32", "processor64")), tr("Follow Proc in Disassembler"), this);
    connect(mActionFollowProc, SIGNAL(triggered()), this, SLOT(followInDisasmSlot()));
    mActionFollowProc->setShortcut(Qt::Key_Return);
    mWindowsTable->addAction(mActionFollowProc);
    mActionToggleProcBP = new QAction(DIcon("breakpoint_toggle"), tr("Toggle Breakpoint in Proc"), this);
    connect(mActionToggleProcBP, SIGNAL(triggered()), this, SLOT(toggleBPSlot()));
    mWindowsTable->addAction(mActionToggleProcBP);
    mActionMessageProcBP = new QAction(DIcon("breakpoint_execute"), tr("Message Breakpoint"), this);
    connect(mActionMessageProcBP, SIGNAL(triggered()), this, SLOT(messagesBPSlot()));

    connect(mHandlesTable, SIGNAL(listContextMenuSignal(QMenu*)), this, SLOT(handlesTableContextMenuSlot(QMenu*)));
    connect(mWindowsTable, SIGNAL(listContextMenuSignal(QMenu*)), this, SLOT(windowsTableContextMenuSlot(QMenu*)));
    connect(mWindowsTable, SIGNAL(enterPressedSignal()), this, SLOT(followInDisasmSlot()));
    connect(mTcpConnectionsTable, SIGNAL(listContextMenuSignal(QMenu*)), this, SLOT(tcpConnectionsTableContextMenuSlot(QMenu*)));
    connect(mPrivilegesTable, SIGNAL(contextMenuSignal(const QPoint &)), this, SLOT(privilegesTableContextMenuSlot(const QPoint &)));
    connect(Config(), SIGNAL(shortcutsUpdated()), this, SLOT(refreshShortcuts()));
    connect(Bridge::getBridge(), SIGNAL(dbgStateChanged(DBGSTATE)), this, SLOT(dbgStateChanged(DBGSTATE)));

#ifdef _WIN32 // This is only supported on Windows Vista or greater
    if(!IsWindowsVistaOrGreater())
#endif //_WIN32
    {
        mTcpConnectionsTable->setRowCount(1);
        mTcpConnectionsTable->setCellContent(0, 0, tr("TCP Connection enumeration is only available on Windows Vista or greater."));
        mTcpConnectionsTable->reloadData();
    }

    mWindowsTable->setAccessibleName(tr("Windows"));
    mHandlesTable->setAccessibleName(tr("Handles"));
    mTcpConnectionsTable->setAccessibleName(tr("TCP Connections"));
    mPrivilegesTable->setAccessibleName(tr("Privileges"));

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
    mActionToggleProcBP->setShortcut(ConfigShortcut("ActionToggleBreakpoint"));
}

void HandlesView::dbgStateChanged(DBGSTATE state)
{
    if(state == stopped)
        reloadData();
}

void HandlesView::handlesTableContextMenuSlot(QMenu* menu)
{
    if(!DbgIsDebugging())
        return;
    auto & table = *mHandlesTable->mCurList;

    menu->addAction(mActionRefresh);
    if(table.getRowCount())
        menu->addAction(mActionCloseHandle);
}

void HandlesView::windowsTableContextMenuSlot(QMenu* menu)
{
    if(!DbgIsDebugging())
        return;
    auto & table = *mWindowsTable->mCurList;
    menu->addAction(mActionRefresh);

    if(table.getRowCount())
    {
        if(table.getCellContent(table.getInitialSelection(), 9) == tr("Enabled"))
        {
            mActionDisableWindow->setText(tr("Disable window"));
            menu->addAction(mActionDisableWindow);
        }
        else
        {
            mActionEnableWindow->setText(tr("Enable window"));
            menu->addAction(mActionEnableWindow);
        }

        menu->addAction(mActionFollowProc);
        menu->addAction(mActionToggleProcBP);
        menu->addAction(mActionMessageProcBP);
    }
}

void HandlesView::tcpConnectionsTableContextMenuSlot(QMenu* menu)
{
    if(!DbgIsDebugging())
        return;
    menu->addAction(mActionRefresh);
}

void HandlesView::privilegesTableContextMenuSlot(const QPoint & pos)
{
    if(!DbgIsDebugging())
        return;
    StdTable & table = *mPrivilegesTable;
    QMenu menu;
    bool isValid = (table.getRowCount() != 0 && table.getCellContent(table.getInitialSelection(), 1) != tr("Unknown"));
    menu.addAction(mActionRefresh);
    if(isValid)
    {
        if(table.getCellContent(table.getInitialSelection(), 1) == tr("Enabled"))
        {
            mActionDisablePrivilege->setText(tr("Disable Privilege: ") + table.getCellContent(table.getInitialSelection(), 0));
            menu.addAction(mActionDisablePrivilege);
        }
        else
        {
            mActionEnablePrivilege->setText(tr("Enable Privilege: ") + table.getCellContent(table.getInitialSelection(), 0));
            menu.addAction(mActionEnablePrivilege);
        }
    }
    menu.addAction(mActionDisableAllPrivileges);
    menu.addAction(mActionEnableAllPrivileges);

    QMenu copyMenu(tr("&Copy"), this);
    copyMenu.setIcon(DIcon("copy"));
    table.setupCopyMenu(&copyMenu);
    if(copyMenu.actions().length())
    {
        menu.addSeparator();
        menu.addMenu(&copyMenu);
    }
    menu.exec(table.mapToGlobal(pos));
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
    for(duint i = 0; i < mPrivilegesTable->getRowCount(); i++)
        if(mPrivilegesTable->getCellContent(i, 1) != tr("Unknown"))
            DbgCmdExecDirect(QString("EnablePrivilege \"%1\"").arg(mPrivilegesTable->getCellContent(i, 0)));
    enumPrivileges();
}

void HandlesView::disableAllPrivilegesSlot()
{
    if(!DbgIsDebugging())
        return;
    for(duint i = 0; i < mPrivilegesTable->getRowCount(); i++)
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
    duint va = 0;
    if(!DbgFunctions()->ValFromString(addrText.toUtf8().constData(), &va))
        return;
    if(!DbgMemIsValidReadPtr(va))
        return;

    BPXTYPE bpType = DbgGetBpxTypeAt(va);
    QString cmd;

    if((bpType & bp_normal) == bp_normal)
        cmd = "bc " + ToPtrString(va);
    else if(bpType == bp_none)
        cmd = "bp " + ToPtrString(va);

    DbgCmdExecDirect(cmd);
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
    QIcon result;
#ifdef _WIN32
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
    if(winIcon != 0)
    {
        result = QIcon(QtWin::fromHICON(winIcon));
        DestroyIcon(winIcon);
    }
#endif //_WIN32
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
    for(int row = 0; row < _countof(PrivilegeString); row++)
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
