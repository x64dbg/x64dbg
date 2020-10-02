#include "StdIconTable.h"

/************************************************************************************
                                Data Management
************************************************************************************/

void StdIconTable::setRowIcon(int r, const QIcon & icon)
{
    mIcon.at(r) = icon;
}

QIcon StdIconTable::getRowIcon(int r) const
{
    return mIcon.at(r);
}

void StdIconTable::setIconColumn(int c)
{
    mIconColumn = c;
}

int StdIconTable::getIconColumn() const
{
    return mIconColumn;
}

void StdIconTable::setRowCount(dsint count)
{
    int wRowToAddOrRemove = count - int(mIcon.size());
    for(int i = 0; i < qAbs(wRowToAddOrRemove); i++)
    {
        if(wRowToAddOrRemove > 0)
            mIcon.push_back(QIcon());
        else
            mIcon.pop_back();
    }
    StdTable::setRowCount(count);
}

void StdIconTable::sortRows(int column, bool ascending)
{
    auto sortFn = mColumnSortFunctions.at(column);
    std::vector<size_t> index;
    index.resize(mData.size());
    size_t i;
    for(i = 0; i < mData.size(); i++)
    {
        index[i] = i;
    }
    std::stable_sort(index.begin(), index.end(), [column, ascending, this, &sortFn](const size_t & a, const size_t & b)
    {
        auto less = sortFn(mData.at(a).at(column).text, mData.at(b).at(column).text);
        return ascending ? less : !less;
    });
    auto copy1 = mData;
    auto copy2 = mIcon;
    for(i = 0; i < mData.size(); i++)
    {
        mData[i] = std::move(copy1[index[i]]);
        mIcon[i] = std::move(copy2[index[i]]);
    }
}

QString StdIconTable::paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h)
{
    if(col == mIconColumn)
    {
        mIcon.at(rowBase + rowOffset).paint(painter, x, y, h, h);
        QString wStr = StdTable::paintContent(painter, rowBase, rowOffset, col, x + h, y, w, h);

        if(wStr.length())
        {
            painter->setPen(getCellColor(rowBase + rowOffset, col));
            painter->drawText(QRect(x + 4 + h, y, w - 4 - h, h), Qt::AlignVCenter | Qt::AlignLeft, wStr);
        }
        return QString();
    }
    else
        return StdTable::paintContent(painter, rowBase, rowOffset, col, x, y, w, h);
}
