#ifndef QGRAPHVIEW_H
#define QGRAPHVIEW_H

#include <QGraphicsView>
#include <QWheelEvent>

class QGraphView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit QGraphView(QWidget* parent = nullptr);
    void wheelEvent(QWheelEvent* event);

private slots:
    void scalingTime(qreal x);
    void animFinished();

private:
    QPoint mMousePos;
    bool bAnimationFinished;
    int mNumScheduledScalings;
};

#endif // QGRAPHVIEW_H
