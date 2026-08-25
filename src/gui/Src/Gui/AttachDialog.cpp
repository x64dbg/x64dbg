#include "AttachDialog.h"
#include "ui_AttachDialog.h"
#include "StdIconSearchListView.h"
#include "StdTable.h"
#include <QMenu>
#include <QMessageBox>
#include <QFileInfo>
#include <QCheckBox>

AttachDialog::AttachDialog(QWidget* parent) : QDialog(parent), ui(new Ui::AttachDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // Setup actions/shortcuts
    //
    // Enter key as shortcut for "Attach"
    mAttachAction = new QAction(DIcon("attach"), tr("Attach"), this);
    mAttachAction->setShortcut(QKeySequence("enter"));
    connect(mAttachAction, SIGNAL(triggered()), this, SLOT(on_btnAttach_clicked()));

    // F5 as shortcut to refresh view
    mRefreshAction = new QAction(DIcon("arrow-restart"), tr("Refresh"), this);
    mRefreshAction->setShortcut(ConfigShortcut("ActionRefresh"));
    ui->btnRefresh->setText(tr("Refresh") + QString(" (%1)").arg(mRefreshAction->shortcut().toString()));
    connect(mRefreshAction, SIGNAL(triggered()), this, SLOT(refresh()));
    this->addAction(mRefreshAction);
    connect(ui->btnRefresh, SIGNAL(clicked()), this, SLOT(refresh()));

    // Create search view (regex disabled)
    mSearchListView = new StdIconSearchListView(this, false, false);
    mSearchListView->mSearchStartCol = 0;
    ui->verticalLayout->insertWidget(0, mSearchListView);

    //setup process list
    int charwidth = mSearchListView->getCharWidth();
    mSearchListView->addColumnAt(charwidth * sizeof(int) * 2 + 8, tr("PID"), true, QString(), StdTable::SortBy::AsInt);
    mSearchListView->addColumnAt(150, tr("Name"), true);
    mSearchListView->addColumnAt(300, tr("Title"), true);
    mSearchListView->addColumnAt(500, tr("Path"), true);
    mSearchListView->addColumnAt(800, tr("Command Line Arguments"), true);
    mSearchListView->setIconColumn(1); // Name
    mSearchListView->setDrawDebugOnly(false);

    connect(mSearchListView, SIGNAL(enterPressedSignal()), this, SLOT(on_btnAttach_clicked()));
    connect(mSearchListView, SIGNAL(listContextMenuSignal(QMenu*)), this, SLOT(processListContextMenu(QMenu*)));

    // Highlight the search box
    mSearchListView->mCurList->setFocus();

    Config()->loadWindowGeometry(this);

    // Populate the process list atleast once
    refresh();
}

AttachDialog::~AttachDialog()
{
    Config()->saveWindowGeometry(this);
    delete ui;
}

void AttachDialog::refresh()
{
    mSearchListView->setRowCount(0);
    DBGPROCESSINFO* entries;
    int count;
    if(!DbgFunctions()->GetProcessList(&entries, &count))
        return;
    mSearchListView->setRowCount(count);
    for(int i = 0; i < count; i++)
    {
        QFileInfo fi(entries[i].szExeFile);
        mSearchListView->setCellContent(i, ColPid, QString().sprintf("%u", entries[i].dwProcessId));
        mSearchListView->setCellContent(i, ColName, fi.baseName());
        mSearchListView->setCellContent(i, ColTitle, QString(entries[i].szExeMainWindowTitle));
        mSearchListView->setCellContent(i, ColPath, QString(entries[i].szExeFile));
        mSearchListView->setCellContent(i, ColCommandLine, QString(entries[i].szExeArgs));
        mSearchListView->setRowIcon(i, getFileIcon(QString(entries[i].szExeFile)));
    }
    mSearchListView->reloadData();
    mSearchListView->refreshSearchList();
}

void AttachDialog::on_btnAttach_clicked()
{
    QString pid = mSearchListView->mCurList->getCellContent(mSearchListView->mCurList->getInitialSelection(), ColPid);
    if(!pid.isEmpty())
        attachToProcess(pid.toUInt());
}

void AttachDialog::on_btnFindWindow_clicked()
{
    QString windowText;
retryFindWindow:
    if(!SimpleInputBox(this, tr("Find Window"), windowText, windowText, tr("Enter window title or class name here.")))
        return;
    HWND hWndFound = FindWindowW(NULL, reinterpret_cast<LPCWSTR>(windowText.utf16())); //Window Title first
    if(hWndFound == NULL)
        hWndFound = FindWindowW(reinterpret_cast<LPCWSTR>(windowText.utf16()), NULL); //Then try window class name
    if(hWndFound == NULL)
    {
        QMessageBox retryDialog(QMessageBox::Critical, tr("Find Window"), tr("Cannot find window \"%1\". Retry?").arg(windowText), QMessageBox::Cancel | QMessageBox::Retry, this);
        retryDialog.setWindowIcon(DIcon("compile-error"));
        if(retryDialog.exec() == QMessageBox::Retry)
            goto retryFindWindow;
    }
    else
    {
        DWORD pid, tid;
        if((tid = GetWindowThreadProcessId(hWndFound, &pid)))
        {
            refresh();
            QString pidText = QString().sprintf("%u", pid);
            bool found = false;
            for(duint i = 0; i < mSearchListView->mCurList->getRowCount(); i++)
            {
                if(mSearchListView->mCurList->getCellContent(i, ColPid) == pidText)
                {
                    mSearchListView->mCurList->setSingleSelection(i);
                    found = true;
                    break;
                }
            }
            if(!found)
            {
                QMessageBox hiddenProcessDialog(QMessageBox::Question, tr("Find Window"),
                                                tr("The PID of the window \"%1\" is %2, but it's hidden in the process list. Do you want to attach to it immediately?").arg(windowText).arg(pidText),
                                                QMessageBox::Yes | QMessageBox::No, this);
                if(hiddenProcessDialog.exec() == QMessageBox::Yes)
                {
                    attachToProcess(pid);
                }
            }
        }
        else
            SimpleErrorBox(this, tr("Find Window"), tr("GetWindowThreadProcessId() failed. Cannot get the PID of the window."));
    }
}

void AttachDialog::processListContextMenu(QMenu* menu)
{
    // Don't show menu options if nothing is listed
    if(!mSearchListView->mCurList->getRowCount())
        return;

    menu->addAction(mAttachAction);
    menu->addAction(mRefreshAction);
}

void AttachDialog::attachToProcess(quint32 pid)
{
    // If not currently debugging, just attach directly
    if(!DbgIsDebugging())
    {
        DbgCmdExecAsync(QString("attach .%1").arg(pid));
        accept();
        return;
    }

    // Check if trying to attach to the same process we're already debugging
    if(DbgValFromString("$pid") == pid)
    {
        QMessageBox::information(this, tr("Already attached"), tr("You are already debugging this process."));
        return;
    }

    // Check if we should show the confirmation dialog
    if(ConfigBool("Gui", "ShowAttachConfirmation"))
    {
        // Show confirmation dialog
        auto cb = new QCheckBox(tr("Remember my choice"));
        QMessageBox msgbox(this);
        msgbox.setText(tr("You are already debugging a process. What would you like to do with the current process?"));
        msgbox.setWindowTitle(tr("Already debugging"));
        msgbox.setWindowIcon(DIcon("bug"));

        auto terminateButton = msgbox.addButton(QMessageBox::Yes);
        terminateButton->setText(tr("&Terminate"));
        terminateButton->setToolTip(tr("Terminate the current process and attach to the new one."));

        auto detachButton = msgbox.addButton(QMessageBox::No);
        detachButton->setText(tr("&Detach"));
        detachButton->setToolTip(tr("Detach from the current process (leaving it running) and attach to the new one."));

        auto cancelButton = msgbox.addButton(QMessageBox::Cancel);
        cancelButton->setText(tr("&Cancel"));
        cancelButton->setToolTip(tr("Cancel and don't attach to the new process."));

        msgbox.setDefaultButton(QMessageBox::Cancel);
        msgbox.setEscapeButton(QMessageBox::Cancel);
        msgbox.setCheckBox(cb);

        auto code = msgbox.exec();

        if(code == QMessageBox::Cancel)
            return; // User cancelled

        if(code == QMessageBox::No) // Detach
        {
            // Set DetachOnAttach so backend will detach instead of terminate
            BridgeSettingSetUint("Engine", "DetachOnAttach", 1);
            if(cb->isChecked())
                Config()->setBool("Gui", "ShowAttachConfirmation", false);
        }
        else if(code == QMessageBox::Yes) // Terminate
        {
            // Ensure DetachOnAttach is off so backend will terminate
            BridgeSettingSetUint("Engine", "DetachOnAttach", 0);
            if(cb->isChecked())
                Config()->setBool("Gui", "ShowAttachConfirmation", false);
        }
    }
    // If ShowAttachConfirmation is false, backend uses existing DetachOnAttach setting
    // (set by a previous "Remember my choice" selection)

    DbgCmdExecAsync(QString("attach .%1").arg(pid));
    accept();
}
