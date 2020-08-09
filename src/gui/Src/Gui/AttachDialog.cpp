#include "AttachDialog.h"
#include "ui_AttachDialog.h"
#include "StdIconSearchListView.h"
#include "StdTable.h"
#include <QMenu>
#include <QMessageBox>
#include <QFileInfo>

AttachDialog::AttachDialog(QWidget* parent) : QDialog(parent), ui(new Ui::AttachDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // Setup actions/shortcuts
    //
    // Enter key as shortcut for "Attach"
    mAttachAction = new QAction(tr("Attach"), this);
    mAttachAction->setShortcut(QKeySequence("enter"));
    connect(mAttachAction, SIGNAL(triggered()), this, SLOT(on_btnAttach_clicked()));

    // F5 as shortcut to refresh view
    mRefreshAction = new QAction(tr("Refresh"), this);
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
    mSearchListView->addColumnAt(charwidth * sizeof(int) * 2 + 8, tr("PID"), true, QString(), ConfigBool("Gui", "PidTidInHex") ? StdTable::SortBy::AsHex : StdTable::SortBy::AsInt);
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

    Config()->setupWindowPos(this);

    // Populate the process list atleast once
    refresh();
}

AttachDialog::~AttachDialog()
{
    Config()->saveWindowPos(this);
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
        mSearchListView->setCellContent(i, ColPid, QString().sprintf(ConfigBool("Gui", "PidTidInHex") ? "%.8X" : "%u", entries[i].dwProcessId));
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
    DbgCmdExec(QString("attach %1%2").arg(ConfigBool("Gui", "PidTidInHex") ? "" : ".").arg(pid));
    accept();
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
        retryDialog.setWindowIcon(DIcon("compile-error.png"));
        if(retryDialog.exec() == QMessageBox::Retry)
            goto retryFindWindow;
    }
    else
    {
        DWORD pid, tid;
        if(tid = GetWindowThreadProcessId(hWndFound, &pid))
        {
            refresh();
            QString pidText = QString().sprintf(ConfigBool("Gui", "PidTidInHex") ? "%.8X" : "%u", pid);
            bool found = false;
            for(int i = 0; i < mSearchListView->mCurList->getRowCount(); i++)
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
                    DbgCmdExec(QString("attach %1").arg(pid, 0, 16));
                    accept();
                }
            }
        }
        else
            SimpleErrorBox(this, tr("Find Window"), tr("GetWindowThreadProcessId() failed. Cannot get the PID of the window."));
    }
}

void AttachDialog::processListContextMenu(QMenu* wMenu)
{
    // Don't show menu options if nothing is listed
    if(!mSearchListView->mCurList->getRowCount())
        return;

    wMenu->addAction(mAttachAction);
    wMenu->addAction(mRefreshAction);
}
