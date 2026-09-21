#include "gui/AttachDialog.h"

#include <BasicView/StdSearchListView.h>
#include <Configuration.h>
#include <MiscUtil.h>
#include <QAction>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <unistd.h>
#include <vector>

#include "core/DbgAdapter.h"

AttachDialog::AttachDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Attach"));
    setWindowIcon(DIcon("attach"));
    resize(900, 500);

    mRefreshAction = new QAction(tr("Refresh"), this);
    mRefreshAction->setShortcut(ConfigShortcut("ActionRefresh"));
    connect(mRefreshAction, &QAction::triggered, this, &AttachDialog::onRefresh);
    addAction(mRefreshAction);

    mSearchListView = new StdSearchListView(this, false, false);
    mSearchListView->mSearchStartCol = 0;

    const int charwidth = mSearchListView->getCharWidth();
    mSearchListView->addColumnAt(charwidth * sizeof(int) * 2 + 8, tr("PID"), true, QString(), StdTable::SortBy::AsInt);
    mSearchListView->addColumnAt(150, tr("Name"), true);
    mSearchListView->addColumnAt(500, tr("Path"), true);
    mSearchListView->addColumnAt(800, tr("Command Line Arguments"), true);
    mSearchListView->setDrawDebugOnly(false);

    connect(mSearchListView, &SearchListView::enterPressedSignal, this, &AttachDialog::onAttach);

    const auto refreshButton = new QPushButton(tr("Refresh") + QStringLiteral(" (%1)").arg(mRefreshAction->shortcut().toString()), this);
    connect(refreshButton, &QPushButton::clicked, this, &AttachDialog::onRefresh);

    const auto attachButton = new QPushButton(tr("&Attach"), this);
    attachButton->setDefault(true);
    connect(attachButton, &QPushButton::clicked, this, &AttachDialog::onAttach);

    const auto cancelButton = new QPushButton(tr("&Cancel"), this);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    const auto buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(refreshButton);
    buttonLayout->addWidget(attachButton);
    buttonLayout->addWidget(cancelButton);

    const auto mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(mSearchListView);
    mainLayout->addLayout(buttonLayout);

    Config()->loadWindowGeometry(this);

    onRefresh();
}

AttachDialog::~AttachDialog()
{
    Config()->saveWindowGeometry(this);
}

void AttachDialog::onRefresh()
{
    const auto processes = DbgAdapter::enumProcesses();
    const pid_t self = getpid();

    mSearchListView->setRowCount(processes.size());
    duint row = 0;
    for(const auto & p : processes)
    {
        if(p.pid == self)
            continue;

        duint attachable = AttachOk;
        if(p.arch == ElfBugArch_I386)
            attachable = AttachWrongArch;
        else if(p.arch != ElfBugArch_X86_64)
            attachable = AttachUnknownArch;
        else if(p.traced)
            attachable = AttachTraced;

        QString name = QString::fromUtf8(p.name);
        if(attachable == AttachWrongArch)
            name = tr("%1 (32-bit)").arg(name);
        else if(attachable == AttachUnknownArch)
            name = tr("%1 (unknown architecture)").arg(name);
        else if(attachable == AttachTraced)
            name = tr("%1 (being debugged)").arg(name);

        mSearchListView->setCellContent(row, ColPid, QString::number(p.pid));
        mSearchListView->setCellUserdata(row, ColPid, static_cast<duint>(p.pid));
        mSearchListView->setCellContent(row, ColName, name);
        mSearchListView->setCellUserdata(row, ColName, attachable);
        mSearchListView->setCellContent(row, ColPath, QString::fromUtf8(p.path));
        mSearchListView->setCellContent(row, ColCommandLine, QString::fromUtf8(p.command_line));
        row++;
    }
    mSearchListView->setRowCount(row);
    mSearchListView->reloadData();
    mSearchListView->refreshSearchList();
}

void AttachDialog::onAttach()
{
    if(!mSearchListView->mCurList->getRowCount())
        return;
    const duint row = mSearchListView->mCurList->getInitialSelection();
    const duint attachable = mSearchListView->mCurList->getCellUserdata(row, ColName);
    if(attachable != AttachOk)
    {
        QString reason;
        if(attachable == AttachWrongArch)
            reason = tr("This is a 32-bit process, and only 64-bit processes can be debugged.");
        else if(attachable == AttachUnknownArch)
            reason = tr("The architecture of this process could not be determined.");
        else
            reason = tr("This process is already being debugged.");
        QMessageBox::information(this, tr("Cannot attach"), reason);
        return;
    }
    const auto pid = static_cast<pid_t>(mSearchListView->mCurList->getCellUserdata(row, ColPid));
    if(pid <= 0)
        return;
    mSelectedPid = pid;
    accept();
}
