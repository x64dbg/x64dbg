#include "StdSearchListView.h"
#include "StdTable.h"

class StdTableSearchList : public AbstractSearchList
{
public:
    friend class StdSearchListView;

    StdTableSearchList()
    {
        mList = new StdTable();
        mSearchList = new StdTable();
    }

    void lock() override { }
    void unlock() override { }
    AbstractStdTable* list() const override { return mList; }
    AbstractStdTable* searchList() const override { return mSearchList; }

    void filter(const QString & filter, FilterType type, int startColumn) override
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

private:
    StdTable* mList;
    StdTable* mSearchList;
};

StdSearchListView::StdSearchListView(QWidget* parent, bool enableRegex, bool enableLock)
    : SearchListView(parent, mSearchListData = new StdTableSearchList(), enableRegex, enableLock)
{
}

StdSearchListView::~StdSearchListView()
{
    delete mSearchListData;
}

void StdSearchListView::setInternalTitle(const QString & title)
{
    stdList()->setWindowTitle(title);
}

int StdSearchListView::getCharWidth()
{
    return stdList()->getCharWidth();
}

void StdSearchListView::addColumnAt(int width, QString title, bool isClickable)
{
    stdList()->addColumnAt(width, title, isClickable);
    stdSearchList()->addColumnAt(width, title, isClickable);
}

void StdSearchListView::setDrawDebugOnly(bool value)
{
    stdList()->setDrawDebugOnly(value);
    stdSearchList()->setDrawDebugOnly(value);
}

void StdSearchListView::enableMultiSelection(bool enabled)
{
    stdList()->enableMultiSelection(enabled);
    stdSearchList()->enableMultiSelection(enabled);
}

void StdSearchListView::setAddressColumn(int col, bool cipBase)
{
    stdList()->setAddressColumn(col, cipBase);
    stdSearchList()->setAddressColumn(col, cipBase);
}

void StdSearchListView::loadColumnFromConfig(const QString & viewName)
{
    stdList()->loadColumnFromConfig(viewName);
    stdSearchList()->loadColumnFromConfig(viewName);
}

void StdSearchListView::setRowCount(dsint count)
{
    mSearchBox->setText("");
    stdList()->setRowCount(count);
}

void StdSearchListView::setCellContent(int r, int c, QString s)
{
    mSearchBox->setText("");
    stdList()->setCellContent(r, c, s);
}

void StdSearchListView::reloadData()
{
    mSearchBox->setText("");
    stdList()->reloadData();
    stdList()->setFocus();
}

void StdSearchListView::setSearchStartCol(int col)
{
    if(col < stdList()->getColumnCount())
        mSearchStartCol = col;
}

StdTable* StdSearchListView::stdList()
{
    return mSearchListData->mList;
}

StdTable* StdSearchListView::stdSearchList()
{
    return mSearchListData->mSearchList;
}
