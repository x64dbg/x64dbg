#include "SourceViewerManager.h"
#include "Bridge.h"
#include <QFileInfo>
#include <QDir>

SourceViewerManager::SourceViewerManager(QWidget* parent) : QTabWidget(parent)
{
    setMovable(true);
    setTabsClosable(true);

    //Close All Tabs
    mCloseAllTabs = new QPushButton(this);
    mCloseAllTabs->setIcon(DIcon("close-all-tabs.png"));
    mCloseAllTabs->setToolTip(tr("Close All Tabs"));
    connect(mCloseAllTabs, SIGNAL(clicked()), this, SLOT(closeAllTabs()));
    setCornerWidget(mCloseAllTabs, Qt::TopLeftCorner);

    connect(this, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTab(int)));
    connect(Bridge::getBridge(), SIGNAL(loadSourceFile(QString, int, int)), this, SLOT(loadSourceFile(QString, int, int)));
    connect(Bridge::getBridge(), SIGNAL(dbgStateChanged(DBGSTATE)), this, SLOT(dbgStateChanged(DBGSTATE)));
}

void SourceViewerManager::loadSourceFile(QString path, int line, int selection)
{
    if(!line)
        line = selection;
    for(int i = 0; i < count(); i++)
    {
        SourceView* curView = (SourceView*)this->widget(i);
        if(curView->getSourcePath().compare(path, Qt::CaseInsensitive) == 0) //file already loaded
        {
            QWidget* now = QApplication::focusWidget();
            curView->setSelection(line);
            setCurrentIndex(i); //show that loaded tab
            if(now)
                now->setFocus();
            return;
        }
    }
    //check if file actually exists
    if(!QFileInfo(path).exists())
    {
        return; //error?
    }
    //load the new file
    QString title = path;
    int idx = path.lastIndexOf(QDir::separator());
    if(idx != -1)
        title = path.mid(idx + 1);
    SourceView* newView = new SourceView(path, line, this);
    connect(newView, SIGNAL(showCpu()), this, SIGNAL(showCpu()));
    addTab(newView, title);
    setCurrentIndex(count() - 1);
}

void SourceViewerManager::closeTab(int index)
{
    removeTab(index);
}

void SourceViewerManager::closeAllTabs()
{
    clear();
}

void SourceViewerManager::dbgStateChanged(DBGSTATE state)
{
    if(state == stopped)
        closeAllTabs();
}
