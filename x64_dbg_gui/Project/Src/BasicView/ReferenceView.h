#ifndef REFERENCEVIEW_H
#define REFERENCEVIEW_H

#include "SearchListView.h"
#include <QProgressBar>

class ReferenceView : public SearchListView
{
public:
    ReferenceView();

private:
    QProgressBar* mSearchProgress;
};

#endif // REFERENCEVIEW_H
