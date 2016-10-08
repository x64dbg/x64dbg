#include "AttachDialog.h"
#include "ui_AttachDialog.h"
#include "SearchListView.h"
#include <QMenu>

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
    mSearchListView = new SearchListView(false, this);
    mSearchListView->mSearchStartCol = 1;
    ui->verticalLayout->insertWidget(0, mSearchListView);

    //setup process list
    int charwidth = mSearchListView->mList->getCharWidth();
    mSearchListView->mList->addColumnAt(charwidth * sizeof(int) * 2 + 8, tr("PID"), true);
    mSearchListView->mList->addColumnAt(500, tr("Path"), true);
    mSearchListView->mList->addColumnAt(800, tr("Command Line Arguments"), true);
    mSearchListView->mList->setDrawDebugOnly(false);

    charwidth = mSearchListView->mSearchList->getCharWidth();
    mSearchListView->mSearchList->addColumnAt(charwidth * sizeof(int) * 2 + 8, tr("PID"), true);
    mSearchListView->mSearchList->addColumnAt(500, tr("Path"), true);
    mSearchListView->mSearchList->addColumnAt(800, tr("Command Line Arguments"), true);
    mSearchListView->mSearchList->setDrawDebugOnly(false);

    connect(mSearchListView, SIGNAL(enterPressedSignal()), this, SLOT(on_btnAttach_clicked()));
    connect(mSearchListView, SIGNAL(listContextMenuSignal(QMenu*)), this, SLOT(processListContextMenu(QMenu*)));

    // Highlight the search box
    mSearchListView->mCurList->setFocus();

    // Populate the process list atleast once
    refresh();
}

AttachDialog::~AttachDialog()
{
    delete ui;
}

void AttachDialog::refresh()
{
    mSearchListView->mList->setRowCount(0);
    mSearchListView->mList->setTableOffset(0);
    DBGPROCESSINFO* entries;
    int count;
    if(!DbgFunctions()->GetProcessList(&entries, &count))
        return;
    mSearchListView->mList->setRowCount(count);
    for(int i = 0; i < count; i++)
    {
        mSearchListView->mList->setCellContent(i, 0, QString().sprintf(ConfigBool("Gui", "PidInHex") ? "%.8X" : "%u", entries[i].dwProcessId));
        mSearchListView->mList->setCellContent(i, 1, QString(entries[i].szExeFile));
        mSearchListView->mList->setCellContent(i, 2, QString(entries[i].szExeArgs));
    }
    mSearchListView->mList->setSingleSelection(0);
    mSearchListView->mList->reloadData();
    mSearchListView->refreshSearchList();
}

void AttachDialog::on_btnAttach_clicked()
{
    QString pid = mSearchListView->mCurList->getCellContent(mSearchListView->mCurList->getInitialSelection(), 0);
    if(!ConfigBool("Gui", "PidInHex"))
        pid.sprintf("%.8X", pid.toULong());
    DbgCmdExec(QString("attach " + pid).toUtf8().constData());
    accept();
}

void AttachDialog::processListContextMenu(QMenu* wMenu)
{
    // Don't show menu options if nothing is listed
    if(!mSearchListView->mCurList->getRowCount())
        return;

    wMenu->addAction(mAttachAction);
    wMenu->addAction(mRefreshAction);
}
