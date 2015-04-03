#ifndef SOURCEVIEWERMANAGER_H
#define SOURCEVIEWERMANAGER_H

#include <QTabWidget>
#include <QPushButton>

class SourceViewerManager : public QTabWidget
{
    Q_OBJECT
public:
    explicit SourceViewerManager(QWidget* parent = 0);

signals:

public slots:
    void closeAllTabs();

private:
    QPushButton* mCloseAllTabs;
};

#endif // SOURCEVIEWERMANAGER_H
