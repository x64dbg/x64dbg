#include <QFrame>
#include <QTimerEvent>
#include "TraceManager.h"
#include "TraceWidget.h"
#include "TraceBrowser.h"
#include "BrowseDialog.h"
#include "MRUList.h"
#include "StringUtil.h"
#include "MiscUtil.h"
#include "TabBar.h"

TraceManager::TraceManager(QWidget* parent) : MHTabWidget(parent, true, true)
{
    //MRU List
    mMRUList = new MRUList(this, "Recent Trace Files");
    connect(mMRUList, SIGNAL(openFile(QString)), this, SLOT(openSlot(QString)));
    mMRUList->load();

    //Close All Tabs Button
    QPushButton* mCloseAllTabs = new QPushButton(this);
    mCloseAllTabs->setIcon(DIcon("close-all-tabs"));
    mCloseAllTabs->setToolTip(tr("Close All Tabs"));
    connect(mCloseAllTabs, SIGNAL(clicked()), this, SLOT(closeAllTabs()));
    setCornerWidget(mCloseAllTabs, Qt::TopLeftCorner);

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
    addTabEx(newView, DIcon("trace"), path, path); //TODO: Proper title
    int index = count() - 1;
    setCurrentIndex(index);
    mMRUList->addEntry(path);
    mMRUList->save();
    connect(newView, &TraceWidget::closeFile, this, [newView, this]()
    {
        // Find index, it could be moved by the user to another position
        for(int index = 0; index < count(); index++)
        {
            if(widget(index) == newView)
            {
                DeleteTab(index);
                return;
            }
        }
    });
    connect(newView, &TraceWidget::displayLogWidget, this, [this]()
    {
        emit displayLogWidget();
    });
}

void TraceManager::DeleteTab(int index)
{
    auto view = qobject_cast<TraceWidget*>(widget(index));
    if(view)
    {
        view->deleteLater(); // It needs to return from close event before we can delete
    }
    MHTabWidget::DeleteTab(index); // Tell the parent class to close the tab
}

void TraceManager::closeAllTabs()
{
    while(count())
    {
        DeleteTab(count() - 1);
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
