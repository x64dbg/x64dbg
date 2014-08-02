#ifndef REFERENCEVIEW_H
#define REFERENCEVIEW_H

#include <QProgressBar>
#include <QAction>
#include <QMenu>
#include "SearchListView.h"
#include "Bridge.h"

class ReferenceView : public SearchListView
{
    Q_OBJECT

public:
    ReferenceView();
    void setupContextMenu();

private slots:
    void addColumnAt(int width, QString title);
    void setRowCount(int_t count);
    void deleteAllColumns();
    void setCellContent(int r, int c, QString s);
    void reloadData();
    void setSingleSelection(int index, bool scroll);
    void setSearchStartCol(int col);
    void referenceContextMenu(QMenu* wMenu);
    void followAddress();
    void followDumpAddress();
    void followGenericAddress();
    void toggleBreakpoint();
    void toggleBookmark();
    void refreshShortcutsSlot();

signals:
    void showCpu();

private:
    QProgressBar* mSearchProgress;
    QAction* mFollowAddress;
    QAction* mFollowDumpAddress;
    QAction* mToggleBreakpoint;
    QAction* mToggleBookmark;
    bool mFollowDumpDefault;
};

#endif // REFERENCEVIEW_H
