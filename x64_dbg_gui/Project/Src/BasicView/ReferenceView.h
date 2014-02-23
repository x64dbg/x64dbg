#ifndef REFERENCEVIEW_H
#define REFERENCEVIEW_H

#include <QProgressBar>
#include "SearchListView.h"
#include "Bridge.h"

class ReferenceView : public SearchListView
{
    Q_OBJECT

public:
    ReferenceView();

private slots:
    void addColumnAt(int width, QString title);
    void setRowCount(int_t count);
    void deleteAllColumns();
    void setCellContent(int r, int c, QString s);
    void reloadData();
    void setSingleSelection(int index, bool scroll);

private:
    QProgressBar* mSearchProgress;
};

#endif // REFERENCEVIEW_H
