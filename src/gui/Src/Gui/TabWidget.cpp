// Qt includes
#include "tabbar.h"
#include "tabwidget.h"
#include <QMoveEvent>
#include <QApplication>
#include <QDesktopWidget>

//////////////////////////////////////////////////////////////
// Default Constructor
//////////////////////////////////////////////////////////////
MHTabWidget::MHTabWidget(QWidget* parent, bool allowDetach, bool allowDelete) : QTabWidget(parent)
{
    mHistoryPopup = new MultiItemsSelectWindow(this, parentWidget(), true);

    mTabBar = new MHTabBar(this, allowDetach, allowDelete);
    connect(mTabBar, SIGNAL(OnDetachTab(int, const QPoint &)), this, SLOT(DetachTab(int, const QPoint &)));
    connect(mTabBar, SIGNAL(OnMoveTab(int, int)), this, SLOT(MoveTab(int, int)));
    connect(mTabBar, SIGNAL(OnDeleteTab(int)), this, SLOT(DeleteTab(int)));
    connect(mTabBar, SIGNAL(tabMoved(int, int)), this, SLOT(tabMoved(int, int)));
    connect(mTabBar, SIGNAL(currentChanged(int)), this, SLOT(currentChanged(int)));

    setTabBar(mTabBar);
    setMovable(true);
    setStyleSheet("QTabWidget::pane { border: 0px; }");

    mWindows.clear();
}

//////////////////////////////////////////////////////////////
// Default Destructor
//////////////////////////////////////////////////////////////
MHTabWidget::~MHTabWidget()
{
    disconnect(mTabBar, SIGNAL(OnMoveTab(int, int)), this, SLOT(MoveTab(int, int)));
    disconnect(mTabBar, SIGNAL(OnDetachTab(int, const QPoint &)), this, SLOT(DetachTab(int, const QPoint &)));
    disconnect(mTabBar, SIGNAL(OnDeleteTab(int)), this, SLOT(DeleteTab(int)));
    disconnect(mTabBar, SIGNAL(currentChanged(int)), this, SLOT(currentChanged(int)));
    delete mTabBar;
}

QWidget* MHTabWidget::widget(int index) const
{
    int baseCount = QTabWidget::count();

    // Check if it's just a normal tab
    if(index < baseCount)
        return QTabWidget::widget(index);

    // Otherwise it's going to be a window
    return mWindows.at(index - baseCount);
}

int MHTabWidget::count() const
{
    return QTabWidget::count() + mWindows.size();
}

QList<QWidget*> MHTabWidget::windows()
{
    return mWindows;
}

// Add a tab
int MHTabWidget::addTabEx(QWidget* widget, const QIcon & icon, const QString & label, const QString & nativeName)
{
    mNativeNames.append(nativeName);
    mHistory.push_back((MIDPKey)widget);
    return this->addTab(widget, icon, label);
}

// Convert an external window to a widget tab
void MHTabWidget::AttachTab(QWidget* parent)
{
    // Retrieve widget
    MHDetachedWindow* detachedWidget = reinterpret_cast<MHDetachedWindow*>(parent);
    QWidget* tearOffWidget = detachedWidget->centralWidget();

    // Reattach the tab
    addTabEx(tearOffWidget, detachedWidget->windowIcon(), detachedWidget->windowTitle(), detachedWidget->mNativeName);

    // Remove it from the windows list
    for(int i = 0; i < mWindows.size(); i++)
    {
        if(mWindows.at(i) == tearOffWidget)
        {
            mWindows.removeAt(i);
        }
    }

    // Cleanup Window
    disconnect(detachedWidget, SIGNAL(OnClose(QWidget*)), this, SLOT(AttachTab(QWidget*)));
    disconnect(detachedWidget, SIGNAL(OnFocused(QWidget*)), this, SLOT(OnDetachFocused(QWidget*)));
    detachedWidget->hide();
    detachedWidget->close();
}

// Convert a tab to an external window
void MHTabWidget::DetachTab(int index, const QPoint & dropPoint)
{
    Q_UNUSED(dropPoint);
    // Create the window
    MHDetachedWindow* detachedWidget = new MHDetachedWindow(parentWidget(), this);
    detachedWidget->setWindowModality(Qt::NonModal);

    // Find Widget and connect
    connect(detachedWidget, SIGNAL(OnClose(QWidget*)), this, SLOT(AttachTab(QWidget*)));
    connect(detachedWidget, SIGNAL(OnFocused(QWidget*)), this, SLOT(OnDetachFocused(QWidget*)));

    detachedWidget->setWindowTitle(tabText(index));
    detachedWidget->setWindowIcon(tabIcon(index));
    detachedWidget->mNativeName = mNativeNames[index];
    mNativeNames.removeAt(index);

    // Remove from tab bar
    QWidget* tearOffWidget = widget(index);
    tearOffWidget->setParent(detachedWidget);

    // Add it to the windows list
    mWindows.append(tearOffWidget);

    // Make first active
    if(count() > 0)
        setCurrentIndex(0);

    // Create and show
    detachedWidget->setCentralWidget(tearOffWidget);

    // Needs to be done explicitly
    tearOffWidget->showNormal();
    QRect screenGeometry = QApplication::desktop()->screenGeometry();
    int w = 640;
    int h = 480;
    int x = (screenGeometry.width() - w) / 2;
    int y = (screenGeometry.height() - h) / 2;
    detachedWidget->showNormal();
    detachedWidget->setGeometry(x, y, w, h);
    detachedWidget->showNormal();
}

// Swap two tab indices
void MHTabWidget::MoveTab(int fromIndex, int toIndex)
{
    QString nativeName;
    removeTab(fromIndex);
    nativeName = mNativeNames.at(fromIndex);
    mNativeNames.removeAt(fromIndex);
    insertTab(toIndex, widget(fromIndex), tabIcon(fromIndex), tabText(fromIndex));
    mNativeNames.insert(toIndex, nativeName);
    setCurrentIndex(toIndex);
}

// Remove a tab, while still keeping the widget intact
void MHTabWidget::DeleteTab(int index)
{
    QWidget* w = widget(index);
    mHistory.removeAll((MIDPKey)w);
    removeTab(index);
    mNativeNames.removeAt(index);
}

void MHTabWidget::tabMoved(int from, int to)
{
    QString nativeName;
    nativeName = mNativeNames.at(from);
    mNativeNames.removeAt(from);
    mNativeNames.insert(to, nativeName);
    emit tabMovedTabWidget(from, to);
}

void MHTabWidget::OnDetachFocused(QWidget* parent)
{
    MHDetachedWindow* detachedWidget = reinterpret_cast<MHDetachedWindow*>(parent);
    QWidget* tearOffWidget = detachedWidget->centralWidget();
    setLatestFocused(tearOffWidget);
}

void MHTabWidget::currentChanged(int index)
{
    setLatestFocused(widget(index));
}

void MHTabWidget::setCurrentIndex(int index)
{
    // Check if it's just a normal tab
    if(index < QTabWidget::count())
    {
        QTabWidget::setCurrentIndex(index);
    }
    else
    {
        // Otherwise it's going to be a window (just bring it up)
        MHDetachedWindow* window = dynamic_cast<MHDetachedWindow*>(widget(index)->parent());
        window->activateWindow();
        window->showNormal();
        window->setFocus();
    }

    setLatestFocused(widget(index));
}

MHTabBar* MHTabWidget::tabBar() const
{
    return mTabBar;
}

QList<MIDPKey> MHTabWidget::MIDP_getItems()
{
    return mHistory;
}

QString MHTabWidget::MIDP_getItemName(MIDPKey index)
{
    return reinterpret_cast<QWidget*>(index)->windowTitle();
}

void MHTabWidget::MIDP_selected(MIDPKey index)
{
    setLatestFocused((QWidget*)index);

    // check in tabbar
    for(auto i = 0; i < QTabWidget::count(); ++i)
    {
        if(reinterpret_cast<QWidget*>(index) == QTabWidget::widget(i))
        {
            QTabWidget::setCurrentIndex(i);
            parentWidget()->activateWindow();
            parentWidget()->setFocus();
            return;
        }
    }

    // should be a detached window
    MHDetachedWindow* window = dynamic_cast<MHDetachedWindow*>(reinterpret_cast<QWidget*>(index)->parent());
    window->activateWindow();
    window->showNormal();
    window->setFocus();
}

QIcon MHTabWidget::MIDP_getIcon(MIDPKey index)
{
    // check in tabbar
    for(auto i = 0; i < QTabWidget::count(); ++i)
    {
        if(reinterpret_cast<QWidget*>(index) == QTabWidget::widget(i))
        {
            return mTabBar->tabIcon(i);
        }
    }

    // should be a detached window
    MHDetachedWindow* window = dynamic_cast<MHDetachedWindow*>(reinterpret_cast<QWidget*>(index)->parent());
    return window->windowIcon();
}

void MHTabWidget::setLatestFocused(QWidget* w)
{
    mHistory.removeAll((MIDPKey)w);
    mHistory.push_front((MIDPKey)w);
}

QString MHTabWidget::getNativeName(int index)
{
    if(index < QTabWidget::count())
    {
        return mNativeNames.at(index);
    }
    else
    {
        MHDetachedWindow* window = dynamic_cast<MHDetachedWindow*>(mWindows.at(index - QTabWidget::count())->parent());
        if(window)
            return window->mNativeName;
        else
            return QString();
    }
}

void MHTabWidget::showPreviousTab()
{
    if(QTabWidget::count() <= 1)
    {
        return;
    }

    int previousTabIndex = QTabWidget::currentIndex();
    if(previousTabIndex == 0)
    {
        previousTabIndex = QTabWidget::count() - 1;
    }
    else
    {
        previousTabIndex--;
    }

    QTabWidget::setCurrentIndex(previousTabIndex);
}

void MHTabWidget::showNextTab()
{
    if(QTabWidget::count() <= 1)
    {
        return;
    }

    QTabWidget::setCurrentIndex((QTabWidget::currentIndex() + 1) % QTabWidget::count());
}

void MHTabWidget::showPreviousView()
{
    mHistoryPopup->gotoPreviousItem();
}

void MHTabWidget::showNextView()
{
    mHistoryPopup->gotoNextItem();
}

void MHTabWidget::deleteCurrentTab()
{
    if(QTabWidget::count() == 0)
    {
        return;
    }

    int index = QTabWidget::currentIndex();
    DeleteTab(index);
    if(index < count())
    {
        // open the tab to the right of the deleted tab
        setCurrentIndex(index);
    }
}

//----------------------------------------------------------------------------

MHDetachedWindow::MHDetachedWindow(QWidget* parent, MHTabWidget* tabwidget) : QMainWindow(parent)
{
    mTabWidget = tabwidget;
}

MHDetachedWindow::~MHDetachedWindow()
{
}

void MHDetachedWindow::closeEvent(QCloseEvent* event)
{
    Q_UNUSED(event);

    emit OnClose(this);
}

bool MHDetachedWindow::event(QEvent* event)
{
    if(event->type() == QEvent::WindowActivate && this->isActiveWindow())
    {
        emit OnFocused(this);
    }
    return QMainWindow::event(event);
}
