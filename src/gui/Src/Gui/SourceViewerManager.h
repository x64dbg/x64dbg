#pragma once

#include <QTabWidget>
#include <QPushButton>
#include <QMap>
#include "SourceView.h"

class SourceViewerManager : public QTabWidget
{
    Q_OBJECT
public:
    explicit SourceViewerManager(QWidget* parent = 0);

public slots:
    void loadSourceFile(QString path, duint addr);
    void closeTab(int index);
    void closeAllTabs();
    void dbgStateChanged(DBGSTATE state);

private:
    QPushButton* mCloseAllTabs;
};
