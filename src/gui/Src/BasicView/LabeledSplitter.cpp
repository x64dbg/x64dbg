#include "LabeledSplitter.h"
#include <QPainter>
#include <QpaintEvent>

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
    painter.drawText(textRect, 0, parent->getName(index));
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

void LabeledSplitter::insertWidget(int index, QWidget* widget, const QString & name)
{
    names.insert(index, name);
    insertWidget(index, widget);
}

void LabeledSplitter::insertWidget(int index, QWidget* widget)
{
    QSplitter::insertWidget(index, widget);
}

QString LabeledSplitter::getName(int index) const
{
    return names.at(index);
}
