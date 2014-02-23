#include "ReferenceView.h"

ReferenceView::ReferenceView()
{
    // Get font information
    QFont wFont("Monospace", 8);
    wFont.setStyleHint(QFont::Monospace);
    wFont.setFixedPitch(true);
    int charwidth=QFontMetrics(wFont).width(QChar(' '));

    // Setup example list
    mList->addColumnAt(charwidth*2*sizeof(int_t)+8, "Address", true);
    mList->addColumnAt(0, "Data", true);
    mSearchList->addColumnAt(charwidth*2*sizeof(int_t)+8, "Address", true);
    mSearchList->addColumnAt(0, "Data", true);

    // Create search progress bar
    mSearchProgress = new QProgressBar();
    mSearchProgress->setRange(0, 100);
    mSearchProgress->setTextVisible(false);
    mSearchProgress->setMaximumHeight(15);

    // Add the progress bar to the main layout
    mMainLayout->addWidget(mSearchProgress);
}
