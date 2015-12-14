#ifndef _GRAPH_NODE_H
#define _GRAPH_NODE_H

#include <QWidget>
#include <QMessageBox>
#include <QFontMetrics>

class GraphNode : public QWidget
{
public:
    GraphNode()
    {
    }

    GraphNode(QString label)
    {
        setLabel(label);
        setStyleSheet("border: 1px solid blue");
        setContentsMargins(0,0,0,0);
    }

    GraphNode(const GraphNode & other)
    {
        setLabel(other._label);
    }

    GraphNode & operator=(const GraphNode & other)
    {
        setLabel(other._label);
        return *this;
    }

    QRectF boundingRect() const
    {
        return QRectF(0, 0, _cachedWidth, _cachedHeight);
    }

    void paintEvent(QPaintEvent* event)
    {
        Q_UNUSED(event);

        QPainter painter(this);

        painter.save();

        //draw bounding rectangle
        QRectF rect = boundingRect();
        painter.setPen(Qt::red);
        //painter.drawRect(rect);

        //draw node contents
        painter.setPen(Qt::black);
        painter.setFont(this->_font);
        QRect textRect = QRect(_spacingX, _spacingY, rect.width() - _spacingX, rect.height() - _spacingY);
        painter.drawText(textRect, _label);

        painter.restore();
    }

    void mousePressEvent(QMouseEvent* event)
    {
        Q_UNUSED(event);

        QMessageBox::information(nullptr, "clicked", _label);
    }

    void setLabel(const QString & label)
    {
        _label = label;
        updateCache();
    }

    void updateCache()
    {
        QFontMetrics metrics(this->_font);
        _cachedWidth = metrics.width(this->_label) + _spacingX * 2;
        _cachedHeight = metrics.height() + _spacingY * 2;
    }

    QString label()
    {
        return _label;
    }

private:
    QString _label;
    QFont _font = QFont("Lucida Console", 8, QFont::Normal, false);
    const qreal _spacingX = 3;
    const qreal _spacingY = 3;
    qreal _cachedWidth;
    qreal _cachedHeight;
};

#endif //_GRAPH_NODE_H
