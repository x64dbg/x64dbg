#include "ReferenceView.h"

ReferenceView::ReferenceView()
{
    // Get font information
    QFont wFont("Monospace", 8);
    wFont.setStyleHint(QFont::Monospace);
    wFont.setFixedPitch(true);
    int charwidth=QFontMetrics(wFont).width(QChar(' '));

    // Setup example list
    mList->addColumnAt(charwidth*2*sizeof(int_t)+8, "Address", false);
    mList->addColumnAt(0, "Data", true);
    mSearchList->addColumnAt(charwidth*2*sizeof(int_t)+8, "Address", false);
    mSearchList->addColumnAt(0, "Data", true);
}
