#include "LabeledSplitter.h"
#include "LabeledSplitterDetachedWindow.h"
#include <QPainter>
#include <QpaintEvent>
#include <QMenu>
#include <QDesktopWidget>
#include <QApplication>
#include "Bridge.h"
#include "Configuration.h"

//LabeledSplitterHandle class
LabeledSplitterHandle::LabeledSplitterHandle(Qt::Orientation o, LabeledSplitter* parent) : QSplitterHandle(o, parent)
{
    charHeight = QFontMetrics(font()).height();
    setMouseTracking(true);
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

void LabeledSplitter::setupContextMenu()
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
    getParent()->currentIndex = getIndex();
    QSplitterHandle::contextMenuEvent(event);
    return;
}

void LabeledSplitter::collapseSlot()
{
    auto sizes = this->sizes();
    int index2;
    int* originalSize = &(qobject_cast<LabeledSplitterHandle*>(handle(currentIndex))->originalSize);
    for(index2 = currentIndex - 1; sizes.at(index2) == 0 && index2 != 0; index2--);
    if(sizes.at(currentIndex) == 0)
    {
        if(*originalSize == 0)
            *originalSize = 100;
        if(sizes[index2] > *originalSize)
            sizes[index2] -= *originalSize;
        sizes[currentIndex] = *originalSize;
    }
    else
    {
        *originalSize = sizes[currentIndex];
        sizes[currentIndex] = 0;
        sizes[index2] += *originalSize;
    }
    setSizes(sizes);
}

// Convert a tab to an external window
void LabeledSplitter::detachSlot()
{

    // Create the window
    LabeledSplitterDetachedWindow* detachedWidget = new LabeledSplitterDetachedWindow(this, this);
    detachedWidget->setWindowModality(Qt::NonModal);

    // Find Widget and connect
    connect(detachedWidget, SIGNAL(OnClose(LabeledSplitterDetachedWindow*)), this, SLOT(attachSlot(LabeledSplitterDetachedWindow*)));

    detachedWidget->setWindowTitle(mNames.at(currentIndex));
    detachedWidget->index = currentIndex;
    // Remove from splitter
    QWidget* tearOffWidget = widget(currentIndex);
    tearOffWidget->setParent(detachedWidget);

    // Add it to the windows list
    mWindows.append(tearOffWidget);

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
    mNames.removeAt(currentIndex);
}

void LabeledSplitter::attachSlot(LabeledSplitterDetachedWindow* widget)
{
    // Retrieve widget
    QWidget* tearOffWidget = widget->centralWidget();

    // Remove it from the windows list
    for(int i = 0; i < mWindows.size(); i++)
    {
        if(mWindows.at(i) == tearOffWidget)
        {
            mWindows.removeAt(i);
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
    painter.drawText(textRect, 0, parent->mNames.at(index));
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
    if(event->x() <= charHeight && event->button() & Qt::LeftButton)
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
    setupContextMenu();
}

QSplitterHandle* LabeledSplitter::createHandle()
{
    return new LabeledSplitterHandle(orientation(), this);
}

void LabeledSplitter::addWidget(QWidget* widget, const QString & name)
{
    mNames.push_back(name);
    addWidget(widget);
}

void LabeledSplitter::addWidget(QWidget* widget)
{
    QSplitter::addWidget(widget);
}

void LabeledSplitter::collapseLowerTabs()
{
    if(count() > 3)
    {
        auto size = sizes();
        size[0] = 1;
        size[1] = 1;
        size[2] = 1;
        for(int i = 3; i < size.count(); i++)
            size[i] = 0;
        setSizes(size);
    }
}

void LabeledSplitter::insertWidget(int index, QWidget* widget, const QString & name)
{
    mNames.insert(index, name);
    insertWidget(index, widget);
}

void LabeledSplitter::insertWidget(int index, QWidget* widget)
{
    QSplitter::insertWidget(index, widget);
}

void LabeledSplitter::contextMenuEvent(QContextMenuEvent* event)
{
    event->accept();
    if(sizes().at(currentIndex) != 0)
        mExpandCollapseAction->setText(tr("&Collapse"));
    else
        mExpandCollapseAction->setText(tr("&Expand"));
    mMenu->exec(mapToGlobal(event->pos()));
}

void LabeledSplitter::loadFromConfig(const QString & configName)
{
    if(!configName.isEmpty())
    {
        mConfigName = configName;
        char state[MAX_SETTING_SIZE];
        memset(state, 0, sizeof(state));
        BridgeSettingGet("Gui", mConfigName.toUtf8().constData(), state);
        size_t sizeofState = strlen(state);
        if(sizeofState > 0)
            this->restoreState(QByteArray::fromBase64(QByteArray(state, int(sizeofState))));
        connect(Bridge::getBridge(), SIGNAL(close()), this, SLOT(closeSlot()));
    }
}

void LabeledSplitter::closeSlot()
{
    if(Config()->getBool("Gui", "SaveColumnOrder"))
        BridgeSettingSet("Gui", mConfigName.toUtf8().constData(), this->saveState().toBase64().data());
}
