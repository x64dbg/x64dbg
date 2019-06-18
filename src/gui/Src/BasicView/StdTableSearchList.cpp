#include "StdTableSearchList.h"

void StdTableSearchList::filter(const QString & filter, FilterType type, int startColumn)
{
    mSearchList->setRowCount(0);
    int rows = mList->getRowCount();
    int columns = mList->getColumnCount();
    for(int i = 0, j = 0; i < rows; i++)
    {
        if(rowMatchesFilter(filter, type, i, startColumn))
        {
            mSearchList->setRowCount(j + 1);
            for(int k = 0; k < columns; k++)
            {
                mSearchList->setCellContent(j, k, mList->getCellContent(i, k));
                mSearchList->setCellUserdata(j, k, mList->getCellUserdata(i, k));
            }
            j++;
        }
    }
}
