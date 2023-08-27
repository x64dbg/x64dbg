#include "StdTable.h"
#include "Bridge.h"

StdTable::StdTable(QWidget* parent) : AbstractStdTable(parent)
{
}

/************************************************************************************
                                   Sorting
************************************************************************************/
bool StdTable::SortBy::AsText(const QString & a, const QString & b)
{
    return QString::compare(a, b) < 0;
}

bool StdTable::SortBy::AsInt(const QString & a, const QString & b)
{
    return a.toLongLong() < b.toLongLong();
}

bool StdTable::SortBy::AsHex(const QString & a, const QString & b)
{
    return a.toLongLong(0, 16) < b.toLongLong(0, 16);
}

/************************************************************************************
                                Data Management
************************************************************************************/
void StdTable::addColumnAt(int width, QString title, bool isClickable, QString copyTitle, SortBy::t sortFn)
{
    AbstractTableView::addColumnAt(width, title, isClickable);

    //append empty column to list of rows
    for(size_t i = 0; i < mData.size(); i++)
        mData[i].push_back(CellData());

    //Append copy title
    if(!copyTitle.length())
        mCopyTitles.push_back(title);
    else
        mCopyTitles.push_back(copyTitle);

    //append column sort function
    mColumnSortFunctions.push_back(sortFn);
}

void StdTable::deleteAllColumns()
{
    setRowCount(0);
    AbstractTableView::deleteAllColumns();
    mCopyTitles.clear();
    mColumnSortFunctions.clear();
}

void StdTable::setRowCount(duint count)
{
    auto oldSize = mData.size();
    mData.resize(count);
    if(oldSize < count)
    {
        for(duint i = oldSize; i < count; i++)
        {
            mData[i].resize(getColumnCount());
        }
    }
    AbstractTableView::setRowCount(count);
}

void StdTable::setCellContent(duint r, duint c, QString s)
{
    if(isValidIndex(r, c))
        mData[r][c].text = std::move(s);
}

void StdTable::setCellContent(duint r, duint c, QString s, duint userdata)
{
    if(isValidIndex(r, c))
    {
        mData[r][c].text = std::move(s);
        mData[r][c].userdata = userdata;
    }
}

QString StdTable::getCellContent(duint r, duint c)
{
    if(isValidIndex(r, c))
        return mData[r][c].text;
    else
        return QString("");
}

void StdTable::setCellUserdata(duint r, duint c, duint userdata)
{
    if(isValidIndex(r, c))
        mData[r][c].userdata = userdata;
}

duint StdTable::getCellUserdata(duint r, duint c)
{
    return isValidIndex(r, c) ? mData[r][c].userdata : 0;
}

bool StdTable::isValidIndex(duint r, duint c)
{
    if(r < 0 || c < 0 || r >= int(mData.size()))
        return false;
    return c < int(mData.at(r).size());
}

void StdTable::sortRows(duint column, bool ascending)
{
    auto sortFn = mColumnSortFunctions.at(column);
    std::stable_sort(mData.begin(), mData.end(), [column, ascending, &sortFn](const std::vector<CellData> & a, const std::vector<CellData> & b)
    {
        auto less = sortFn(a.at(column).text, b.at(column).text);
        return ascending ? less : !less;
    });
}
