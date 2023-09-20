#pragma once

#include <QTabWidget>
#include <QPushButton>
#include "ReferenceView.h"

class ReferenceManager : public QTabWidget
{
    Q_OBJECT
public:
    explicit ReferenceManager(QWidget* parent = nullptr);
    ReferenceView* currentReferenceView();

private slots:
    void newReferenceView(QString name);
    void closeTab(int index);
    void closeAllTabs();

signals:
    void showCpu();

private:
    ReferenceView* mCurrentReferenceView;
    QPushButton* mCloseAllTabs;
};
