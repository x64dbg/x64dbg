#include "StdIconTable.h"

/************************************************************************************
                                Data Management
************************************************************************************/

void StdIconTable::setRowIcon(duint r, const QIcon & icon)
{
    mIcon.at(r) = icon;
}

QIcon StdIconTable::getRowIcon(duint r) const
{
    return mIcon.at(r);
}

void StdIconTable::setIconColumn(duint c)
{
    mIconColumn = c;
}

duint StdIconTable::getIconColumn() const
{
    return mIconColumn;
}

void StdIconTable::setRowCount(duint count)
{
    mIcon.resize(count);
    StdTable::setRowCount(count);
}

void StdIconTable::sortRows(duint column, bool ascending)
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

QString StdIconTable::paintContent(QPainter* painter, duint row, duint col, int x, int y, int w, int h)
{
    if(col == mIconColumn)
    {
        // Draw the selection first, so that transparent icons are drawn properly
        if(isSelected(row))
            painter->fillRect(QRect(x, y, w, h), QBrush(mSelectionColor));

        mIcon.at(row).paint(painter, x, y, h, h);
        QString str = StdTable::paintContent(painter, row, col, x + h, y, w - h, h);

        if(str.length())
        {
            painter->setPen(getCellColor(row, col));
            painter->drawText(QRect(x + 4 + h, y, w - 4 - h, h), Qt::AlignVCenter | Qt::AlignLeft, str);
        }
        return QString();
    }
    else
        return StdTable::paintContent(painter, row, col, x, y, w, h);
}
