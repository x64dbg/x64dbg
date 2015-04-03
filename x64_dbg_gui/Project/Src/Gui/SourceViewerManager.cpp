#include "SourceViewerManager.h"

SourceViewerManager::SourceViewerManager(QWidget* parent) : QTabWidget(parent)
{
    setMovable(true);
    setTabsClosable(true);

    //Close All Tabs
    mCloseAllTabs = new QPushButton(this);
    mCloseAllTabs->setIcon(QIcon(":/icons/images/close-all-tabs.png"));
    mCloseAllTabs->setToolTip("Close All Tabs");
    connect(mCloseAllTabs, SIGNAL(clicked()), this, SLOT(closeAllTabs()));
    setCornerWidget(mCloseAllTabs, Qt::TopLeftCorner);
}

void SourceViewerManager::closeAllTabs()
{
    clear();
}
