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

	m_Windows.clear();
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

int MHTabWidget::count() const
{
	return QTabWidget::count() + m_Windows.size();
}

QWidget *MHTabWidget::widget(int index) const
{
	int baseCount = QTabWidget::count();

	// Check if it's just a normal tab
	if (index < baseCount)
		return QTabWidget::widget(index);

	// Otherwise it's going to be a window
	return m_Windows.at(index - baseCount);
}

void MHTabWidget::setCurrentIndex(int index)
{
	// Check if it's just a normal tab
	if (index < QTabWidget::count())
	{
		QTabWidget::setCurrentIndex(index);
	}
	else
	{
		// Otherwise it's going to be a window (just bring it up)
		MHDetachedWindow* window = dynamic_cast<MHDetachedWindow*>(widget(index)->parent());
		window->activateWindow();
	}
}

void MHTabWidget::setCurrentWidget(QWidget *widget)
{
	widget = 0;
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
void MHTabWidget::DetachTab(int index, QPoint& dropPoint)
{
	// Create the window
	MHDetachedWindow* detachedWidget = new MHDetachedWindow(parentWidget());
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
	if (count() > 0)
		setCurrentIndex(0);

	// Create and show
	detachedWidget->setCentralWidget(tearOffWidget);

	// Needs to be done explicitly
	tearOffWidget->show();
	detachedWidget->setGeometry(dropPoint.x(), dropPoint.y(), 640, 480);
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

	// Remove it from the windows list
	for(int i = 0; i < m_Windows.size(); i++)
	{
		if (m_Windows.at(i) == tearOffWidget)
			m_Windows.removeAt(i);
	}

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

