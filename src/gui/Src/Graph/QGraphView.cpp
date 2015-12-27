#include "QGraphView.h"
#include <QTimeLine>
#include <iostream>

QGraphView::QGraphView(QWidget* parent)
    : QGraphicsView(parent),
      mNumScheduledScalings(0)
{
    bAnimationFinished = false;
}

void QGraphView::wheelEvent(QWheelEvent* event)
{
    if(!(event->modifiers() & Qt::ControlModifier))
    {
        QGraphicsView::wheelEvent(event);
        return;
    }

    int numDegrees = event->delta() / 8;
    int numSteps = numDegrees / 15; // see QWheelEvent documentation
    mNumScheduledScalings += numSteps;
    if(mNumScheduledScalings * numSteps < 0)  // if user moved the wheel in another direction, we reset previously scheduled scalings
        mNumScheduledScalings = numSteps;

    QTimeLine* anim = new QTimeLine(350, this);
    anim->setUpdateInterval(20);

    connect(anim, SIGNAL(valueChanged(qreal)), SLOT(scalingTime(qreal)));
    connect(anim, SIGNAL(finished()), SLOT(animFinished()));

    // Center the view on the mouse cursor before zooming, more convenient zoom
    QPointF mappedMousePos = this->mapToScene(event->pos().x(), event->pos().y());
    if(scene()->itemsBoundingRect().contains(mappedMousePos.x(), mappedMousePos.y()))
        centerOn(mappedMousePos.x(), mappedMousePos.y());

    anim->start();
    bAnimationFinished = false;
}

void QGraphView::scalingTime(qreal x)
{
    Q_UNUSED(x)
    qreal factor = 1.0 + qreal(mNumScheduledScalings) / 300.0;
    scale(factor, factor);
}

void QGraphView::animFinished()
{
    if(mNumScheduledScalings > 0)
        mNumScheduledScalings--;
    else
        mNumScheduledScalings++;

    sender()->deleteLater();
    bAnimationFinished = true;
}
