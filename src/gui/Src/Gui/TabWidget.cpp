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
    m_historyPopup = new HistoryViewsPopupWindow(this, parentWidget());

    m_tabBar = new MHTabBar(this, allowDetach, allowDelete);
    connect(m_tabBar, SIGNAL(OnDetachTab(int, const QPoint &)), this, SLOT(DetachTab(int, const QPoint &)));
    connect(m_tabBar, SIGNAL(OnMoveTab(int, int)), this, SLOT(MoveTab(int, int)));
    connect(m_tabBar, SIGNAL(OnDeleteTab(int)), this, SLOT(DeleteTab(int)));
    connect(m_tabBar, SIGNAL(tabMoved(int, int)), this, SLOT(tabMoved(int, int)));
    connect(m_tabBar, SIGNAL(currentChanged(int)), this, SLOT(currentChanged(int)));

    setTabBar(m_tabBar);
    setMovable(true);
    setStyleSheet("QTabWidget::pane { border: 0px; }");

    m_Windows.clear();
}

//////////////////////////////////////////////////////////////
// Default Destructor
//////////////////////////////////////////////////////////////
MHTabWidget::~MHTabWidget(void)
{
    disconnect(m_tabBar, SIGNAL(OnMoveTab(int, int)), this, SLOT(MoveTab(int, int)));
    disconnect(m_tabBar, SIGNAL(OnDetachTab(int, const QPoint &)), this, SLOT(DetachTab(int, const QPoint &)));
    disconnect(m_tabBar, SIGNAL(OnDeleteTab(int)), this, SLOT(DeleteTab(int)));
    disconnect(m_tabBar, SIGNAL(currentChanged(int)), this, SLOT(currentChanged(int)));
    delete m_tabBar;
}

QWidget* MHTabWidget::widget(int index) const
{
    int baseCount = QTabWidget::count();

    // Check if it's just a normal tab
    if(index < baseCount)
        return QTabWidget::widget(index);

    // Otherwise it's going to be a window
    return m_Windows.at(index - baseCount);
}

int MHTabWidget::count() const
{
    return QTabWidget::count() + m_Windows.size();
}

QList<QWidget*> MHTabWidget::windows()
{
    return m_Windows;
}

// Add a tab
int MHTabWidget::addTabEx(QWidget* widget, const QIcon & icon, const QString & label, const QString & nativeName)
{
    mNativeNames.append(nativeName);
    m_history.push_back(widget);
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
    for(int i = 0; i < m_Windows.size(); i++)
    {
        if(m_Windows.at(i) == tearOffWidget)
        {
            m_Windows.removeAt(i);
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
    m_Windows.append(tearOffWidget);

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
    m_history.removeAll(w);
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
    return m_tabBar;
}

const QList<HPKey> & MHTabWidget::HP_getItems() const
{
    return m_history;
}

QString MHTabWidget::HP_getName(HPKey index)
{
    return index->windowTitle();
}

void MHTabWidget::HP_selected(HPKey index)
{
    setLatestFocused(index);

    // check in tabbar
    for(auto i = 0; i < QTabWidget::count(); ++i)
    {
        if(index == QTabWidget::widget(i))
        {
            QTabWidget::setCurrentIndex(i);
            parentWidget()->activateWindow();
            parentWidget()->setFocus();
            return;
        }
    }

    // should be a detached window
    MHDetachedWindow* window = dynamic_cast<MHDetachedWindow*>(index->parent());
    window->activateWindow();
    window->showNormal();
    window->setFocus();
}

QIcon MHTabWidget::HP_getIcon(HPKey index)
{
    // check in tabbar
    for(auto i = 0; i < QTabWidget::count(); ++i)
    {
        if(index == QTabWidget::widget(i))
        {
            return m_tabBar->tabIcon(i);
        }
    }

    // should be a detached window
    MHDetachedWindow* window = dynamic_cast<MHDetachedWindow*>(index->parent());
    return window->windowIcon();
}

void MHTabWidget::setLatestFocused(QWidget* w)
{
    m_history.removeAll(w);
    m_history.push_front(w);
}

QString MHTabWidget::getNativeName(int index)
{
    if(index < QTabWidget::count())
    {
        return mNativeNames.at(index);
    }
    else
    {
        MHDetachedWindow* window = dynamic_cast<MHDetachedWindow*>(m_Windows.at(index - QTabWidget::count())->parent());
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
    m_historyPopup->gotoPreviousHistory();
}

void MHTabWidget::showNextView()
{
    m_historyPopup->gotoNextHistory();
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
    m_TabWidget = tabwidget;
}

MHDetachedWindow::~MHDetachedWindow(void)
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
