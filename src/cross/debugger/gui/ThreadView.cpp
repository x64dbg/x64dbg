#include "ThreadView.h"

#include <QAction>
#include <QDateTime>
#include <QLocale>
#include <QMenu>
#include <QPainter>
#include <sched.h>

#include "Configuration.h"
#include "MiscUtil.h"
#include "StringUtil.h"

static QString formatDuration(const uint64_t ms)
{
    const uint64_t days = ms / (1000ull * 60 * 60 * 24);
    const QTime time = QTime::fromMSecsSinceStartOfDay(static_cast<int>(ms % (1000ull * 60 * 60 * 24)));
    const QString clock = time.toString(QStringLiteral("HH:mm:ss.zzz"));
    return days ? QStringLiteral("%1:%2").arg(days).arg(clock) : clock;
}

static QString formatCreationTime(const uint64_t ms)
{
    if(ms == 0)
        return QString();
    const QDateTime when = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(ms));
    const QString clock = when.time().toString(QStringLiteral("HH:mm:ss.zzz"));
    if(when.date() == QDate::currentDate())
        return clock;
    return QLocale().toString(when.date(), QLocale::ShortFormat) + ' ' + clock;
}

static QString formatPriority(const DbgThreadInfo & t)
{
    switch(t.policy)
    {
    case SCHED_OTHER:
        return t.nice == 0 ? ThreadView::tr("Normal") : ThreadView::tr("Nice %1").arg(t.nice);
    case SCHED_BATCH:
        return ThreadView::tr("Batch");
    case SCHED_IDLE:
        return ThreadView::tr("Idle");
    case SCHED_FIFO:
        return ThreadView::tr("FIFO %1").arg(t.rtPriority);
    case SCHED_RR:
        return ThreadView::tr("RR %1").arg(t.rtPriority);
    case 6:
        return ThreadView::tr("Deadline");
    default:
        return ThreadView::tr("Unknown");
    }
}

ThreadView::ThreadView(DbgAdapter* adapter, QWidget* parent)
    : StdTable(parent)
    , mAdapter(adapter)
{
    const int charwidth = getCharWidth();
    // TODO: Entry and Last Error columns once ElfBugThreadInfo can supply them.
    addColumnAt(8 + charwidth * sizeof(unsigned int) * 2, tr("Number"), true, "", SortBy::AsInt);
    addColumnAt(8 + charwidth * sizeof(unsigned int) * 2, tr("ID"), true, "", SortBy::AsInt);
    addColumnAt(8 + charwidth * sizeof(duint) * 2, tr("FS Base"), true, "", SortBy::AsHex);
    addColumnAt(8 + charwidth * sizeof(duint) * 2, tr("RIP"), true, "", SortBy::AsHex);
    addColumnAt(8 + charwidth * 14, tr("Suspend Count"), true, "", SortBy::AsInt);
    addColumnAt(8 + charwidth * 12, tr("Priority"), true);
    addColumnAt(8 + charwidth * 20, tr("Wait Reason"), true);
    addColumnAt(8 + charwidth * 16, tr("User Time"), true);
    addColumnAt(8 + charwidth * 16, tr("Kernel Time"), true);
    addColumnAt(8 + charwidth * 16, tr("Creation Time"), true);
    addColumnAt(8, tr("Name"), true);
    enableMultiSelection(true);

    setupContextMenu();

    connect(mAdapter, &DbgAdapter::threadsUpdated, this, &ThreadView::onThreadsUpdated, Qt::QueuedConnection);
    connect(mAdapter, &DbgAdapter::processExited, this, &ThreadView::onProcessExited, Qt::QueuedConnection);
    connect(this, &AbstractStdTable::doubleClickedSignal, this, &ThreadView::switchThreadSlot);
    connect(this, &AbstractStdTable::contextMenuSignal, this, &ThreadView::contextMenuSlot);
}

void ThreadView::setupContextMenu()
{
    mContextMenu = new QMenu(this);
    mSwitchAction = mContextMenu->addAction(QIcon(QStringLiteral(":/Default/icons/thread-switch.png")), tr("Switch Thread"), this, &ThreadView::switchThreadSlot);
    mSuspendAction = mContextMenu->addAction(QIcon(QStringLiteral(":/Default/icons/thread-pause.png")), tr("Suspend Thread"), this, &ThreadView::suspendThreadSlot);
    mResumeAction = mContextMenu->addAction(QIcon(QStringLiteral(":/Default/icons/thread-resume.png")), tr("Resume Thread"), this, &ThreadView::resumeThreadSlot);
    mSuspendAllAction = mContextMenu->addAction(QIcon(QStringLiteral(":/Default/icons/thread-pause.png")), tr("Suspend All Threads"), this, &ThreadView::suspendAllSlot);
    mResumeAllAction = mContextMenu->addAction(QIcon(QStringLiteral(":/Default/icons/thread-resume.png")), tr("Resume All Threads"), this, &ThreadView::resumeAllSlot);
    mContextMenu->addSeparator();
    mSetNameAction = mContextMenu->addAction(QIcon(QStringLiteral(":/Default/icons/thread-setname.png")), tr("Set Name"), this, &ThreadView::setNameSlot);
    mContextMenu->addSeparator();
    QMenu* copyMenu = new QMenu(tr("&Copy"), mContextMenu);
    setupCopyMenu(copyMenu);
    mContextMenu->addMenu(copyMenu);
}

QString ThreadView::paintContent(QPainter* painter, duint row, duint col, int x, int y, int w, int h)
{
    QString ret = StdTable::paintContent(painter, row, col, x, y, w, h);
    const auto threadId = static_cast<pid_t>(getCellUserdata(row, ColThreadId));
    if(threadId == mCurrentThreadId && col == ColNumber)
    {
        painter->fillRect(QRect(x, y, w, h), QBrush(ConfigColor("ThreadCurrentBackgroundColor")));
        painter->setPen(QPen(ConfigColor("ThreadCurrentColor")));
        painter->drawText(QRect(x + 4, y, w - 4, h), Qt::AlignVCenter | Qt::AlignLeft, ret);
        ret = "";
    }
    return ret;
}

void ThreadView::onThreadsUpdated(const QVector<DbgThreadInfo> & threads, const pid_t currentTid)
{
    setRowCount(threads.size());
    for(int i = 0; i < threads.size(); ++i)
    {
        const auto & t = threads[i];
        setCellContent(i, ColNumber, t.number == 0 ? tr("Main") : QString::number(t.number));
        setCellContent(i, ColThreadId, QString::number(t.tid), static_cast<duint>(t.tid));
        setCellContent(i, ColFsBase, ToPtrString(t.fsBase));
        setCellContent(i, ColRip, ToPtrString(t.rip));
        setCellContent(i, ColSuspendCount, QString::number(t.suspendCount));
        setCellContent(i, ColPriority, formatPriority(t));
        setCellContent(i, ColWaitReason, t.waitReason);
        setCellContent(i, ColUserTime, formatDuration(t.userTimeMs));
        setCellContent(i, ColKernelTime, formatDuration(t.kernelTimeMs));
        setCellContent(i, ColCreationTime, formatCreationTime(t.startTimeMs));
        setCellContent(i, ColName, t.name);
    }
    mCurrentThreadId = currentTid;
    reloadData();
}

void ThreadView::onProcessExited()
{
    setRowCount(0);
    mCurrentThreadId = 0;
    reloadData();
}

void ThreadView::switchThreadSlot()
{
    if(!getRowCount() || !mAdapter->isActive())
        return;
    const auto tid = static_cast<pid_t>(getCellUserdata(getInitialSelection(), ColThreadId));
    if(tid == 0)
        return;
    mAdapter->switchThread(tid);
}

void ThreadView::setNameSlot()
{
    if(!getRowCount() || !mAdapter->isActive())
        return;
    const duint row = getInitialSelection();
    const auto tid = static_cast<pid_t>(getCellUserdata(row, ColThreadId));
    if(tid == 0)
        return;
    QString name = getCellContent(row, ColName);
    if(!SimpleInputBox(this, tr("Thread name - %1").arg(tid), name, name, QString()))
        return;
    if(!mAdapter->isActive())
        return;
    mAdapter->setThreadName(tid, name);
}

void ThreadView::setSelectedSuspended(const bool suspended)
{
    if(!getRowCount() || !mAdapter->isActive())
        return;
    for(const duint row : getSelection())
    {
        const auto tid = static_cast<pid_t>(getCellUserdata(row, ColThreadId));
        if(tid != 0)
            mAdapter->setThreadSuspended(tid, suspended);
    }
}

void ThreadView::suspendThreadSlot()
{
    setSelectedSuspended(true);
}

void ThreadView::resumeThreadSlot()
{
    setSelectedSuspended(false);
}

void ThreadView::suspendAllSlot()
{
    if(mAdapter->isActive())
        mAdapter->setAllThreadsSuspended(true);
}

void ThreadView::resumeAllSlot()
{
    if(mAdapter->isActive())
        mAdapter->setAllThreadsSuspended(false);
}

void ThreadView::contextMenuSlot(const QPoint & pos) const
{
    if(!getRowCount())
        return;
    const bool active = mAdapter->isActive();
    mSwitchAction->setEnabled(active);
    mSetNameAction->setEnabled(active);
    mSuspendAction->setEnabled(active);
    mResumeAction->setEnabled(active);
    mSuspendAllAction->setEnabled(active);
    mResumeAllAction->setEnabled(active);
    mContextMenu->exec(mapToGlobal(pos));
}
