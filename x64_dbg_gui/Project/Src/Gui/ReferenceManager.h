#ifndef REFERENCEMANAGER_H
#define REFERENCEMANAGER_H

#include "TabWidget.h"
#include "ReferenceView.h"

class ReferenceManager : public MHTabWidget
{
    Q_OBJECT
public:
    explicit ReferenceManager(QWidget* parent = 0);
    ReferenceView* currentReferenceView();

public slots:
    void newReferenceView(QString name);

signals:
    void showCpu();

private:
    ReferenceView* mCurrentReferenceView;
};

#endif // REFERENCEMANAGER_H
