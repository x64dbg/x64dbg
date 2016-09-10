#include "LabeledSplitter.h"
#include "LabeledSplitterDetachedWindow.h"
#include <QPainter>
#include <QpaintEvent>
#include <QMenu>
#include <QDesktopWidget>
#include "bridge.h"

//LabeledSplitterHandle class
LabeledSplitterHandle::LabeledSplitterHandle(Qt::Orientation o, LabeledSplitter* parent) : QSplitterHandle(o, parent)
{
    charHeight = QFontMetrics(font()).height();
    setMouseTracking(true);
    setupContextMenu();
    originalSize = 0;
}

QSize LabeledSplitterHandle::sizeHint() const
{
    QSize size;
    size.setHeight(charHeight + 2);
    return std::move(size);
}

QSize LabeledSplitterHandle::minimumSizeHint() const
{
    return sizeHint();
}

LabeledSplitter* LabeledSplitterHandle::getParent() const
{
    return qobject_cast<LabeledSplitter*>(parent());
}

int LabeledSplitterHandle::getIndex()
{
    return getParent()->indexOf(this);
}

void LabeledSplitterHandle::setupContextMenu()
{
    mMenu = new QMenu(this);
    mExpandCollapseAction = new QAction(this);
    connect(mExpandCollapseAction, SIGNAL(triggered()), this, SLOT(collapseSlot()));
    mMenu->addAction(mExpandCollapseAction);
    QAction* mDetach = new QAction(tr("&Detach"), this);
    connect(mDetach, SIGNAL(triggered()), this, SLOT(detachSlot()));
    mMenu->addAction(mDetach);
}

void LabeledSplitterHandle::contextMenuEvent(QContextMenuEvent* event)
{
    event->accept();
    LabeledSplitter* parent = getParent();
    int index = parent->indexOf(this);
    if(parent->sizes().at(index) != 0)
        mExpandCollapseAction->setText(tr("&Collapse"));
    else
        mExpandCollapseAction->setText(tr("&Expand"));
    mMenu->exec(mapToGlobal(event->pos()));
}

void LabeledSplitterHandle::collapseSlot()
{
    QMouseEvent event(QMouseEvent::MouseButtonPress, QPointF(0, 0), Qt::LeftButton, Qt::LeftButton, 0);
    mousePressEvent(&event);
}

// Convert a tab to an external window
void LabeledSplitterHandle::detachSlot()
{
    auto parent = getParent();
    int index = parent->indexOf(this);
    // Create the window
    LabeledSplitterDetachedWindow* detachedWidget = new LabeledSplitterDetachedWindow(parent, parent);
    detachedWidget->setWindowModality(Qt::NonModal);

    // Find Widget and connect
    parent->connect(detachedWidget, SIGNAL(OnClose(LabeledSplitterDetachedWindow*)), parent, SLOT(attachSlot(LabeledSplitterDetachedWindow*)));

    detachedWidget->setWindowTitle(parent->names.at(index));
    detachedWidget->index = index;
    // Remove from splitter
    QWidget* tearOffWidget = parent->widget(index);
    tearOffWidget->setParent(detachedWidget);

    // Add it to the windows list
    parent->m_Windows.append(tearOffWidget);

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
    parent->names.removeAt(index);
}

void LabeledSplitter::attachSlot(LabeledSplitterDetachedWindow* widget)
{
    // Retrieve widget
    QWidget* tearOffWidget = widget->centralWidget();

    // Remove it from the windows list
    for(int i = 0; i < m_Windows.size(); i++)
    {
        if(m_Windows.at(i) == tearOffWidget)
        {
            m_Windows.removeAt(i);
        }
    }

    // Make Active
    insertWidget(widget->index, tearOffWidget, widget->windowTitle());

    // Cleanup Window
    disconnect(widget, SIGNAL(OnClose(QWidget*)), this, SLOT(attachSlot(LabeledSplitterDetachedWindow*)));
    widget->hide();
    widget->close();
}

void LabeledSplitterHandle::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    const QRect & rect = event->rect();
    painter.fillRect(rect, Qt::gray);
    painter.setPen(Qt::black);
    LabeledSplitter* parent = getParent();
    int index = parent->indexOf(this);
    if(parent->sizes().at(index) != 0)
    {
        //hidden
        QPoint points[3] = {QPoint(charHeight / 5, charHeight / 3), QPoint(charHeight * 4 / 5, charHeight / 3), QPoint(charHeight / 2, charHeight * 2 / 3)};
        painter.setPen(Qt::black);
        painter.setBrush(Qt::green);
        painter.drawConvexPolygon(points, 3);
    }
    else
    {
        QPoint points[3] = {QPoint(charHeight / 5, charHeight * 2 / 3), QPoint(charHeight * 4 / 5, charHeight * 2 / 3), QPoint(charHeight / 2, charHeight / 3)};
        painter.setPen(Qt::black);
        painter.setBrush(Qt::green);
        painter.drawConvexPolygon(points, 3);
        painter.drawLine(rect.left(), rect.height() - 1, rect.right(), rect.height() - 1);
    }
    QRect textRect(rect.left() + charHeight, rect.top(), rect.width() - charHeight, rect.height());
    painter.drawText(textRect, 0, parent->names.at(index));
}

void LabeledSplitterHandle::mouseMoveEvent(QMouseEvent* event)
{
    if(event->x() <= charHeight)
    {
        this->setCursor(QCursor(Qt::ArrowCursor));
        event->accept();
    }
    else
    {
        this->setCursor(QCursor(Qt::SplitVCursor));
        QSplitterHandle::mouseMoveEvent(event);
    }
}

void LabeledSplitterHandle::mousePressEvent(QMouseEvent* event)
{
    if(event->x() <= charHeight)
    {
        LabeledSplitter* parent = getParent();
        auto sizes = parent->sizes();
        int index = parent->indexOf(this);
        int index2;
        for(index2 = index - 1; sizes.at(index2) == 0 && index2 != 0; index2--);
        if(sizes.at(index) == 0)
        {
            if(originalSize == 0)
                originalSize = 100;
            if(sizes[index2] > originalSize)
                sizes[index2] -= originalSize;
            sizes[index] = originalSize;
        }
        else
        {
            originalSize = sizes[index];
            sizes[index] = 0;
            sizes[index2] += originalSize;
        }
        parent->setSizes(sizes);
        event->accept();
    }
    else
        QSplitterHandle::mousePressEvent(event);
}

// LabeledSplitter class
LabeledSplitter::LabeledSplitter(QWidget* parent) : QSplitter(Qt::Vertical, parent)
{

}

QSplitterHandle* LabeledSplitter::createHandle()
{
    return new LabeledSplitterHandle(orientation(), this);
}

void LabeledSplitter::addWidget(QWidget* widget, const QString & name)
{
    names.push_back(name);
    addWidget(widget);
}

void LabeledSplitter::addWidget(QWidget* widget)
{
    QSplitter::addWidget(widget);
}

void LabeledSplitter::collapseLowerTabs()
{
    if(count() > 2)
    {
        auto size = sizes();
        size[0] = 1;
        size[1] = 1;
        for(int i = 2; i < size.count(); i++)
            size[i] = 0;
        setSizes(size);
    }
}

void LabeledSplitter::insertWidget(int index, QWidget* widget, const QString & name)
{
    names.insert(index, name);
    insertWidget(index, widget);
}

void LabeledSplitter::insertWidget(int index, QWidget* widget)
{
    QSplitter::insertWidget(index, widget);
}
