#include "TraceManager.h"
#include "MiscUtil.h"

TraceManager::TraceManager(QWidget* parent) : QTabWidget(parent)
{
    setMovable(true);
    setTabsClosable(true);

    //Open
    mOpen = new QPushButton(this);
    mOpen->setIcon(DIcon("control-record")); //TODO: New icon
    mOpen->setToolTip(tr("Open"));
    connect(mOpen, SIGNAL(clicked()), this, SLOT(open()));
    setCornerWidget(mOpen, Qt::TopRightCorner);

    //Close All Tabs
    mCloseAllTabs = new QPushButton(this);
    mCloseAllTabs->setIcon(DIcon("close-all-tabs"));
    mCloseAllTabs->setToolTip(tr("Close All Tabs"));
    connect(mCloseAllTabs, SIGNAL(clicked()), this, SLOT(closeAllTabs()));
    setCornerWidget(mCloseAllTabs, Qt::TopLeftCorner);

    connect(this, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTab(int)));

    open();
}

void TraceManager::open()
{
    //load the new file
    TraceWidget* newView = new TraceWidget(this);
    addTab(newView, tr("Trace")); //TODO: Proper title
    setCurrentIndex(count() - 1);
}

void TraceManager::closeTab(int index)
{
    auto view = qobject_cast<TraceWidget*>(widget(index));
    removeTab(index);
    if(view)
        delete view;
}

void TraceManager::closeAllTabs()
{
    while(count())
    {
        closeTab(0);
    }
}
