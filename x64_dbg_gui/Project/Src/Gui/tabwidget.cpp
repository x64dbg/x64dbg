// Qt includes
#include "tabbar.h"
#include "tabwidget.h"

//////////////////////////////////////////////////////////////
// Default Constructor
//////////////////////////////////////////////////////////////
MHTabWidget::MHTabWidget(QWidget *parent) : QTabWidget(parent)
{
	m_tabBar = new MHTabBar(this);
	connect(m_tabBar, SIGNAL(OnDetachTab(int, QPoint&)), this, SLOT(DetachTab(int, QPoint&)));
	connect(m_tabBar, SIGNAL(OnMoveTab(int, int)), this, SLOT(MoveTab(int, int)));

	setTabBar(m_tabBar);
	setMovable(true);
}

//////////////////////////////////////////////////////////////
// Default Destructor
//////////////////////////////////////////////////////////////
MHTabWidget::~MHTabWidget(void)
{
	disconnect(m_tabBar, SIGNAL(OnMoveTab(int, int)), this, SLOT(MoveTab(int, int)));
	disconnect(m_tabBar, SIGNAL(OnDetachTab(int, QPoint&)), this, SLOT(DetachTab(int, QPoint&)));
	delete m_tabBar;
}

//////////////////////////////////////////////////////////////////////////////
void MHTabWidget::MoveTab(int fromIndex, int toIndex)
{
	removeTab(fromIndex);
	insertTab(toIndex, widget(fromIndex), tabIcon(fromIndex), tabText(fromIndex));
	setCurrentIndex(toIndex);
}

//////////////////////////////////////////////////////////////////////////////
void MHTabWidget::DetachTab(int index, QPoint& /*dropPoint*/)
{
	// Create the window
	MHDetachedWindow* detachedWidget = new MHDetachedWindow(parentWidget());
	detachedWidget->setWindowModality(Qt::NonModal);

	// Find Widget and connect
	connect(detachedWidget, SIGNAL(OnClose(QWidget*)), this, SLOT(AttachTab(QWidget*)));

	detachedWidget->setWindowTitle(tabText (index));
	detachedWidget->setWindowIcon(tabIcon(index));

	// Remove from tab bar
	QWidget* tearOffWidget = widget(index);
	tearOffWidget->setParent(detachedWidget);

	// Make first active
	if (count() > 0)
		setCurrentIndex(0);

	// Create and show
	detachedWidget->setCentralWidget(tearOffWidget);

	// Needs to be done explicitly
	tearOffWidget->show();
	detachedWidget->resize(640, 480);
	detachedWidget->show();
}

//////////////////////////////////////////////////////////////////////////////
void MHTabWidget::AttachTab(QWidget *parent)
{
	// Retrieve widget
	MHDetachedWindow* detachedWidget = dynamic_cast<MHDetachedWindow*>(parent);
	QWidget* tearOffWidget = detachedWidget->centralWidget();

	// Change parent
	tearOffWidget->setParent(this);

	// Reattach the tab
	int newIndex = addTab(tearOffWidget, detachedWidget->windowIcon(), detachedWidget->windowTitle());

	// Make Active
	if (newIndex != -1)
		setCurrentIndex(newIndex);

	// Cleanup Window
	disconnect(detachedWidget, SIGNAL(OnClose(QWidget*)), this, SLOT(AttachTab(QWidget*)));
	delete detachedWidget;
}

//----------------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////////
MHDetachedWindow::MHDetachedWindow(QWidget *parent) : QMainWindow(parent)
{
}

//////////////////////////////////////////////////////////////////////////////
MHDetachedWindow::~MHDetachedWindow(void)
{
}

//////////////////////////////////////////////////////////////////////////////
void MHDetachedWindow::closeEvent(QCloseEvent* /*event*/)
{
	emit OnClose(this);
}

