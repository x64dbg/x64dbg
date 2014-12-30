#ifndef REFERENCEMANAGER_H
#define REFERENCEMANAGER_H

#include <QTabWidget>
#include "ReferenceView.h"

class ReferenceManager : public QTabWidget
{
    Q_OBJECT
public:
    explicit ReferenceManager(QWidget* parent = 0);
    ReferenceView* currentReferenceView();

private slots:
    void newReferenceView(QString name);
    void closeTab(int index);

signals:
    void showCpu();

private:
    ReferenceView* mCurrentReferenceView;
};

#endif // REFERENCEMANAGER_H
