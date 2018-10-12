#include "StdSearchListView.h"
#include "StdTable.h"
//#include "CachedFontMetrics.h"
#include "DisassemblyPopup.h"

void StdTableMouseMove::mouseMoveEvent(QMouseEvent* event)
{
    emit mouseMoveSignal(event);
}

class StdTableSearchList : public AbstractSearchList
{
public:
    friend class StdSearchListView;

    StdTableSearchList() : mList(new StdTableMouseMove()), mSearchList(new StdTableMouseMove()) { }
    ~StdTableSearchList() {delete mList; delete mSearchList; }

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
    StdTableMouseMove* mList;
    StdTableMouseMove* mSearchList;
};

StdSearchListView::StdSearchListView(QWidget* parent, bool enableRegex, bool enableLock)
    : SearchListView(parent, mSearchListData = new StdTableSearchList(), enableRegex, enableLock)
{
    setAddressColumn(0);
    mDisassemblyPopup = nullptr;
    connect(mSearchListData->mList, SIGNAL(mouseMoveSignal(QMouseEvent*)), this, SLOT(mouseMoveSlot(QMouseEvent*)));
    connect(mSearchListData->mSearchList, SIGNAL(mouseMoveSignal(QMouseEvent*)), this, SLOT(mouseMoveSlot(QMouseEvent*)));
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

void StdSearchListView::mouseMoveSlot(QMouseEvent* event)
{
    StdTable* that;
    that = qobject_cast<StdTable*>(sender());
    if(that)
    {
        int column = that->getColumnIndexFromX(event->x());
        int row = that->getIndexOffsetFromY(that->transY(event->y()));
        duint addr = 0;
        if(row < that->getRowCount())
        {
            bool ok = false;
            QString addrStr = that->getCellContent(row, column);
            GuiAddStatusBarMessage((addrStr + "\n").toUtf8().constData());
#ifdef _WIN64
            addr = addrStr.toULongLong(&ok, 16);
#else //x86
            addr = addrStr.toULong(&ok, 16);
#endif //_WIN64
            if(ok && DbgFunctions()->MemIsCodePage(addr, false))
                ShowDisassemblyPopup(addr, event->x(), event->y());
            else //not a code section, clear addr to reset default behaviour
                addr = 0;
        }
        ShowDisassemblyPopup(addr, event->x(), event->y());
    }
    else
        ShowDisassemblyPopup(0, 0, 0);
}

void StdSearchListView::leaveEvent(QEvent* event)
{
    ShowDisassemblyPopup(0, 0, 0);
}

void StdSearchListView::ShowDisassemblyPopup(duint addr, int x, int y)
{
    if(!addr)
    {
        if(mDisassemblyPopup)
            mDisassemblyPopup->hide();
        return;
    }
    if(!mDisassemblyPopup)
        mDisassemblyPopup = new DisassemblyPopup(this);
    if(mDisassemblyPopup->getAddress() == addr)
        return;
    if(DbgFunctions()->MemIsCodePage(addr, false))
    {
        mDisassemblyPopup->move(mapToGlobal(QPoint(x + 20, y + stdList()->fontMetrics().height() * 2)));
        mDisassemblyPopup->setAddress(addr);
        mDisassemblyPopup->show();
    }
    else
        mDisassemblyPopup->hide();
}
