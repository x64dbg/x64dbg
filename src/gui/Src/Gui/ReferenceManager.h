#ifndef REFERENCEMANAGER_H
#define REFERENCEMANAGER_H

#include <QTabWidget>
#include <QPushButton>
#include "ReferenceView.h"

class ReferenceManager : public QTabWidget
{
    Q_OBJECT
    Q_PROPERTY(int viewId MEMBER m_viewId)
public:
    explicit ReferenceManager(QWidget* parent = 0);
    ReferenceView* currentReferenceView();

private slots:
    void newReferenceView(QString name);
    void closeTab(int index);
    void closeAllTabs();

signals:
    void showCpu();

private:
    int m_viewId;
    ReferenceView* mCurrentReferenceView;
    QPushButton* mCloseAllTabs;
};

#endif // REFERENCEMANAGER_H
