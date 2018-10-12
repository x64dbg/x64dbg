#ifndef STDSEARCHLISTVIEW_H
#define STDSEARCHLISTVIEW_H

#include "SearchListView.h"
#include "StdTable.h"
class DisassemblyPopup;

class StdTableSearchList;

class StdSearchListView : public SearchListView
{
    Q_OBJECT
public:
    StdSearchListView(QWidget* parent, bool enableRegex, bool enableLock);
    ~StdSearchListView() override;

    void setInternalTitle(const QString & title);
    int getCharWidth();
    void addColumnAt(int width, QString title, bool isClickable);
    void setDrawDebugOnly(bool value);
    void enableMultiSelection(bool enabled);
    void setAddressColumn(int col, bool cipBase = false);
    void loadColumnFromConfig(const QString & viewName);

public slots:
    virtual void setRowCount(dsint count);
    void setCellContent(int r, int c, QString s);
    void reloadData();
    void setSearchStartCol(int col);
    void mouseMoveSlot(QMouseEvent* event);

private:
    StdTableSearchList* mSearchListData;
    DisassemblyPopup* mDisassemblyPopup;

    void leaveEvent(QEvent* event);

protected:
    friend class SymbolView;
    friend class Bridge;
    StdTable* stdList();
    StdTable* stdSearchList();
    void ShowDisassemblyPopup(duint addr, int x, int y);
};

//Hacked class to recieve mouse move event. This class must be placed in a header file, otherwise Qt won't create its internal functions.
class StdTableMouseMove : public StdTable
{
    Q_OBJECT
public:
    explicit StdTableMouseMove(QWidget* parent = 0) : StdTable(parent) { }
    void mouseMoveEvent(QMouseEvent* event);
    //int fontHeight() const {return mFontMetrics->height();}

signals:
    void mouseMoveSignal(QMouseEvent* event);
};
#endif // STDSEARCHLISTVIEW_H
