#ifndef REFERENCEVIEW_H
#define REFERENCEVIEW_H

#include <QWidget>
#include <QVBoxLayout>
#include "StdTable.h"

namespace Ui {
    class ReferenceView;
}

class ReferenceView : public QWidget
{
    Q_OBJECT

public:
    explicit ReferenceView(QWidget *parent = 0);
    ~ReferenceView();

private:
    Ui::ReferenceView *ui;
    QVBoxLayout* mMainLayout;
    StdTable* mReferenceList;
};

#endif // REFERENCEVIEW_H
