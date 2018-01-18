#ifndef SEARCHLISTVIEWTABLE_H
#define SEARCHLISTVIEWTABLE_H

#include "StdTable.h"

class SearchListViewTable : public StdTable
{
    Q_OBJECT
public:
    explicit SearchListViewTable(StdTable* parent = 0);
    QString highlightText;
    void updateColors() override;

    void setCipBase(bool cipBase)
    {
        bCipBase = cipBase;
    }

protected:
    QString paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h);

private:
    QColor mCipBackgroundColor;
    QColor mCipColor;
    QColor mBreakpointBackgroundColor;
    QColor mBreakpointColor;
    QColor mHardwareBreakpointBackgroundColor;
    QColor mHardwareBreakpointColor;
    QColor mBookmarkBackgroundColor;
    QColor mBookmarkColor;
    QColor mLabelColor;
    QColor mLabelBackgroundColor;
    QColor mSelectedAddressBackgroundColor;
    QColor mSelectedAddressColor;
    QColor mAddressBackgroundColor;
    QColor mAddressColor;
    QColor mTracedBackgroundColor;
    QColor mTracedSelectedAddressBackgroundColor;
    bool bCipBase;
};

#endif // SEARCHLISTVIEWTABLE_H
