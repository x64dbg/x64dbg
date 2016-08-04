#ifndef QENTROPYVIEW_H
#define QENTROPYVIEW_H

#include <QGraphicsView>

class QGraphicsScene;

class QEntropyView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit QEntropyView(QWidget* parent = 0);
    void InitializeGraph(int penSize = 1);
    void AddGraph(const std::vector<double> & points, QColor color = Qt::black);
    void GraphFile(const QString & fileName, int blockSize, int pointCount, QColor = Qt::black);
    void GraphMemory(const unsigned char* data, int dataSize, int blockSize, int pointCount, QColor = Qt::black);

private:
    QGraphicsScene* mScene;
    QRectF mRect;
    int mPenSize;
};

#endif // QENTROPYVIEW_H
