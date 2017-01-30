#ifndef SOURCEVIEWERMANAGER_H
#define SOURCEVIEWERMANAGER_H

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
    void loadSourceFile(QString path, int line, int selection = 0);
    void closeTab(int index);
    void closeAllTabs();
    void dbgStateChanged(DBGSTATE state);

private:
    QPushButton* mCloseAllTabs;
};

#endif // SOURCEVIEWERMANAGER_H
