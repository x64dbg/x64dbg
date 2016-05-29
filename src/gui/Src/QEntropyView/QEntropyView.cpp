#include "QEntropyView.h"
#include <QFile>
#include "Entropy.h"

QEntropyView::QEntropyView(QWidget* parent)
    : QGraphicsView(parent),
      mRect(QRectF()),
      mPenSize(1)
{
    mScene = new QGraphicsScene(this);
}

void QEntropyView::InitializeGraph(int penSize)
{
    //initialize scene
    qreal width = this->width() - 5;
    qreal height = this->height() - 5;
    mRect = QRectF(25, 10, width - 35, height - 20);
    mPenSize = penSize;
    mScene->clear();

    //draw bounding box
    mScene->addRect(QRectF(1, 1, width, height), QPen(Qt::black));

    //draw scale
    mScene->addLine(15, mRect.top(), 15, mRect.bottom(), QPen(Qt::black, 2));
    const int xBegin = 10;
    const int xEnd = 20;
    qreal intervalY = mRect.height() / 10;
    for(int i = 0; i < 11; i++)
    {
        qreal y = mRect.top() + i * intervalY;
        mScene->addLine(xBegin, y, xEnd, y, QPen(Qt::black, 2));
    }

    //set scene
    setRenderHints(QPainter::Antialiasing);
    setScene(mScene);
}

void QEntropyView::AddGraph(const std::vector<double> & points, QColor color)
{
    int pointCount = (int)points.size();
    if(!pointCount)
        return;
    qreal intervalX = mRect.width() / ((qreal)pointCount - 1);
    qreal intervalY = mRect.height() / 1;
    QPolygonF polyLine;
    for(int i = 0; i < pointCount; i++)
    {
        qreal x = i * intervalX;
        qreal y = points[i] * intervalY;
        QPointF point(mRect.x() + x, mRect.bottom() - y); //y direction is inverted...
        polyLine.append(point);
    }
    QPainterPath path;
    path.addPolygon(polyLine);
    mScene->addPath(path, QPen(color, mPenSize));
}

void QEntropyView::GraphFile(const QString & fileName, int blockSize, int pointCount, QColor color)
{
    QFile file(fileName);
    if(!file.open(QIODevice::ReadOnly))
        return;

    QByteArray fileData = file.readAll();
    file.close();

    GraphMemory((unsigned char*)fileData.constData(), fileData.size(), blockSize, pointCount, color);
}

void QEntropyView::GraphMemory(const unsigned char* data, int dataSize, int blockSize, int pointCount, QColor color)
{
    std::vector<double> points;
    if(dataSize < blockSize)
    {
        blockSize = dataSize / 2;
        if(!blockSize)
            blockSize = 1;
    }
    if(dataSize < pointCount)
        pointCount = (int)dataSize;
    Entropy::MeasurePoints(data, dataSize, blockSize, points, pointCount);
    AddGraph(points, color);
}
