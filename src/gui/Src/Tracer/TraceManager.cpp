#include <QFrame>
#include <QTimerEvent>
#include "TraceManager.h"
#include "TraceBrowser.h"
#include "BrowseDialog.h"
#include "MRUList.h"
#include "StringUtil.h"
#include "MiscUtil.h"

TraceManager::TraceManager(QWidget* parent) : QTabWidget(parent)
{
    setMovable(true);

    //MRU
    mMRUList = new MRUList(this, "Recent Trace Files");
    connect(mMRUList, SIGNAL(openFile(QString)), this, SLOT(openSlot(QString)));
    mMRUList->load();

    //Close All Tabs
    mCloseAllTabs = new QPushButton(this);
    mCloseAllTabs->setIcon(DIcon("close-all-tabs"));
    mCloseAllTabs->setToolTip(tr("Close All Tabs"));
    connect(mCloseAllTabs, SIGNAL(clicked()), this, SLOT(closeAllTabs()));
    setCornerWidget(mCloseAllTabs, Qt::TopLeftCorner);

    connect(this, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTab(int)));
    connect(Bridge::getBridge(), SIGNAL(openTraceFile(const QString &)), this, SLOT(openSlot(const QString &)));
}

TraceManager::~TraceManager()
{
    closeAllTabs();
}

void TraceManager::open()
{
    BrowseDialog browse(
        this,
        tr("Open trace recording"),
        tr("Trace recording"),
        tr("Trace recordings (*.%1);;All files (*.*)").arg(ArchValue("trace32", "trace64")),
        getDbPath(),
        false
    );
    if(browse.exec() != QDialog::Accepted)
        return;
    openSlot(browse.path);
}

void TraceManager::openSlot(const QString & path)
{
    //load the new file
    TraceWidget* newView = new TraceWidget(Bridge::getArchitecture(), path, this);
    addTab(newView, path); //TODO: Proper title
    int index = count() - 1;
    setCurrentIndex(index);
    mMRUList->addEntry(path);
    mMRUList->save();
    connect(newView, &TraceWidget::closeFile, this, [index, this]()
    {
        closeTab(index);
    });
    connect(newView, &TraceWidget::displayLogWidget, this, [this]()
    {
        emit displayLogWidget();
    });
}

void TraceManager::closeTab(int index)
{
    auto view = qobject_cast<TraceWidget*>(widget(index));
    if(view)
    {
        removeTab(index);
        view->deleteLater(); // It needs to return from close event before we can delete
    }
}

void TraceManager::closeAllTabs()
{
    while(count())
    {
        closeTab(count() - 1);
    }
}

void TraceManager::toggleTraceRecording()
{
    TraceBrowser::toggleTraceRecording(this);
}

void TraceManager::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu wMenu;
    QAction traceCoverageToggleTraceRecording(tr("Start recording"), this);
    if(TraceBrowser::isRecording())
    {
        traceCoverageToggleTraceRecording.setText(tr("Stop trace recording"));
        traceCoverageToggleTraceRecording.setIcon(DIcon("control-stop"));
    }
    else
    {
        traceCoverageToggleTraceRecording.setText(tr("Start trace recording"));
        traceCoverageToggleTraceRecording.setIcon(DIcon("control-record"));
    }
    connect(&traceCoverageToggleTraceRecording, SIGNAL(triggered()), this, SLOT(toggleTraceRecording()));
    // Disable toggle trace when not debugging
    if(!DbgIsDebugging())
        traceCoverageToggleTraceRecording.setEnabled(false);
    wMenu.addAction(&traceCoverageToggleTraceRecording);

    QAction openAction(DIcon("folder-horizontal-open"), tr("Open"), this);
    connect(&openAction, SIGNAL(triggered()), this, SLOT(open()));
    wMenu.addAction(&openAction);

    QMenu wMRUMenu(tr("Recent Files"));
    mMRUList->appendMenu(&wMRUMenu);
    wMenu.addMenu(&wMRUMenu);

    wMenu.exec(mapToGlobal(event->pos()));
}
