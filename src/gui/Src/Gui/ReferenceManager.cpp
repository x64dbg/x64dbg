#include "ReferenceManager.h"
#include "Bridge.h"

ReferenceManager::ReferenceManager(QWidget* parent) : QTabWidget(parent)
{
    setMovable(true);
    setTabsClosable(true);
    mCurrentReferenceView = 0;

    //Close All Tabs
    mCloseAllTabs = new QPushButton(this);
    mCloseAllTabs->setIcon(DIcon("close-all-tabs.png"));
    mCloseAllTabs->setToolTip(tr("Close All Tabs"));
    connect(mCloseAllTabs, SIGNAL(clicked()), this, SLOT(closeAllTabs()));
    setCornerWidget(mCloseAllTabs, Qt::TopLeftCorner);
    setContextMenuPolicy(Qt::CustomContextMenu);

    connect(Bridge::getBridge(), SIGNAL(referenceInitialize(QString)), this, SLOT(newReferenceView(QString)));
    connect(this, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTab(int)));
}

ReferenceView* ReferenceManager::currentReferenceView()
{
    return mCurrentReferenceView;
}

void ReferenceManager::newReferenceView(QString name)
{
    if(mCurrentReferenceView) //disconnect previous reference view
        mCurrentReferenceView->disconnectBridge();
    mCurrentReferenceView = new ReferenceView(false, this);
    mCurrentReferenceView->connectBridge();
    connect(mCurrentReferenceView, SIGNAL(showCpu()), this, SIGNAL(showCpu()));
    insertTab(0, mCurrentReferenceView, name);
    setCurrentIndex(0);
    Bridge::getBridge()->setResult(BridgeResult::RefInitialize, 1);
}

void ReferenceManager::closeTab(int index)
{
    removeTab(index);
    if(count() <= 0)
        emit showCpu();
}

void ReferenceManager::closeAllTabs()
{
    clear();
    emit showCpu();
}
