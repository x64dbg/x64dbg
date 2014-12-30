#include "ReferenceManager.h"
#include "Bridge.h"

ReferenceManager::ReferenceManager(QWidget* parent) : QTabWidget(parent)
{
    setMovable(true);
    setTabsClosable(true);
    mCurrentReferenceView = 0;
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
    mCurrentReferenceView = new ReferenceView();
    connect(mCurrentReferenceView, SIGNAL(showCpu()), this, SIGNAL(showCpu()));
    insertTab(0, mCurrentReferenceView, name);
    setCurrentIndex(0);
    Bridge::getBridge()->BridgeSetResult(1);
}

void ReferenceManager::closeTab(int index)
{
    removeTab(index);
}
