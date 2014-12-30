// Qt includes
#include "tabbar.h"
#include "tabwidget.h"

//////////////////////////////////////////////////////////////
// Default Constructor
//////////////////////////////////////////////////////////////
MHTabWidget::MHTabWidget(QWidget* parent, bool allowDetach, bool allowDelete) : QTabWidget(parent)
{
    m_tabBar = new MHTabBar(this, allowDetach, allowDelete);
    connect(m_tabBar, SIGNAL(OnDetachTab(int, QPoint &)), this, SLOT(DetachTab(int, QPoint &)));
    connect(m_tabBar, SIGNAL(OnMoveTab(int, int)), this, SLOT(MoveTab(int, int)));
    connect(m_tabBar, SIGNAL(OnDeleteTab(int)), this, SLOT(DeleteTab(int)));

    setTabBar(m_tabBar);
    setMovable(true);

    m_Windows.clear();
}

QTabBar* MHTabWidget::tabBar()
{
    return m_tabBar;
}

//////////////////////////////////////////////////////////////
// Default Destructor
//////////////////////////////////////////////////////////////
MHTabWidget::~MHTabWidget(void)
{
    disconnect(m_tabBar, SIGNAL(OnMoveTab(int, int)), this, SLOT(MoveTab(int, int)));
    disconnect(m_tabBar, SIGNAL(OnDetachTab(int, QPoint &)), this, SLOT(DetachTab(int, QPoint &)));
    disconnect(m_tabBar, SIGNAL(OnDeleteTab(int)), this, SLOT(DeleteTab(int)));
    delete m_tabBar;
}

int MHTabWidget::count() const
{
    return QTabWidget::count() + m_Windows.size();
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
}

void MHTabWidget::setCurrentWidget(QWidget* widget)
{
    Q_UNUSED(widget);
    // To be implemented.
}

//////////////////////////////////////////////////////////////////////////////
void MHTabWidget::MoveTab(int fromIndex, int toIndex)
{
    removeTab(fromIndex);
    insertTab(toIndex, widget(fromIndex), tabIcon(fromIndex), tabText(fromIndex));
    setCurrentIndex(toIndex);
}

//////////////////////////////////////////////////////////////////////////////
void MHTabWidget::DetachTab(int index, QPoint & dropPoint)
{
    Q_UNUSED(dropPoint);
    // Create the window
    MHDetachedWindow* detachedWidget = new MHDetachedWindow(parentWidget(), this);
    detachedWidget->setWindowModality(Qt::NonModal);

    // Find Widget and connect
    connect(detachedWidget, SIGNAL(OnClose(QWidget*)), this, SLOT(AttachTab(QWidget*)));

    detachedWidget->setWindowTitle(tabText(index));
    detachedWidget->setWindowIcon(tabIcon(index));

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

void MHTabWidget::DeleteTab(int index)
{
    removeTab(index);
}

//////////////////////////////////////////////////////////////////////////////
void MHTabWidget::AttachTab(QWidget* parent)
{
    // Retrieve widget
    MHDetachedWindow* detachedWidget = reinterpret_cast<MHDetachedWindow*>(parent);
    QWidget* tearOffWidget = detachedWidget->centralWidget();

    // Reattach the tab
    int newIndex = addTab(tearOffWidget, detachedWidget->windowIcon(), detachedWidget->windowTitle());

    // Remove it from the windows list
    for(int i = 0; i < m_Windows.size(); i++)
    {
        if(m_Windows.at(i) == tearOffWidget)
            m_Windows.removeAt(i);
    }

    // Make Active
    if(newIndex != -1)
        setCurrentIndex(newIndex);

    // Cleanup Window
    disconnect(detachedWidget, SIGNAL(OnClose(QWidget*)), this, SLOT(AttachTab(QWidget*)));
    detachedWidget->hide();
    detachedWidget->close();
}

//----------------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////////
MHDetachedWindow::MHDetachedWindow(QWidget* parent, MHTabWidget* tabwidget) : QMainWindow(parent)
{
    m_TabWidget = tabwidget;
}

//////////////////////////////////////////////////////////////////////////////
MHDetachedWindow::~MHDetachedWindow(void)
{
}

/*
//////////////////////////////////////////////////////////////////////////////
void MHDetachedWindow::moveEvent(QMoveEvent *event)
{
    QRect rect = m_TabWidget->geometry();
    QSize hint = m_TabWidget->tabBar()->sizeHint();

    // Height of the actual top tab bar
    rect.setBottom(rect.top() + hint.height());
    QPoint pos1(rect.x(), rect.y());
    pos1=m_TabWidget->mapToGlobal(pos1);
    rect.setX(pos1.x());
    rect.setY(pos1.y());

    if (rect.contains(event->pos()))
    {
        m_TabWidget->AttachTab(this);
        event->accept();
    }
    else
    {
        QMainWindow::moveEvent(event);
    }
}
*/

//////////////////////////////////////////////////////////////////////////////
void MHDetachedWindow::closeEvent(QCloseEvent* /*event*/)
{
    emit OnClose(this);
}
