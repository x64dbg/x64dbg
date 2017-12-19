#ifndef STDTABLE_H
#define STDTABLE_H

#include "AbstractTableView.h"

class StdTable : public AbstractTableView
{
    Q_OBJECT
public:
    explicit StdTable(QWidget* parent = 0);
    QString paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h);
    void reloadData();

    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

    void enableMultiSelection(bool enabled);
    void enableColumnSorting(bool enabled);

    // Selection Management
    void expandSelectionUpTo(int to);
    void expandUp();
    void expandDown();
    void expandTop();
    void expandBottom();
    void setSingleSelection(int index);
    int getInitialSelection();
    QList<int> getSelection();
    void selectStart();
    void selectEnd();
    void selectNext();
    void selectPrevious();
    void selectAll();
    bool isSelected(int base, int offset);
    bool scrollSelect(int offset);

    // Sorting
    struct SortBy
    {
        typedef std::function<bool(const QString &, const QString &)> t;
        static bool AsText(const QString & a, const QString & b);
        static bool AsInt(const QString & a, const QString & b);
        static bool AsHex(const QString & a, const QString & b);
    };

    // Data Management
    void addColumnAt(int width, QString title, bool isClickable, QString copyTitle = "", SortBy::t sortFn = SortBy::AsText);
    void deleteAllColumns() override;
    void setRowCount(dsint count) override;
    void setCellContent(int r, int c, QString s);
    QString getCellContent(int r, int c);
    void setCellUserdata(int r, int c, duint userdata);
    duint getCellUserdata(int r, int c);
    bool isValidIndex(int r, int c);

    //context menu helpers
    void setupCopyMenu(QMenu* copyMenu);
    void setupCopyMenu(MenuBuilder* copyMenu);
    void setCopyMenuOnly(bool bSet, bool bDebugOnly = true);

signals:
    void selectionChangedSignal(int index);
    void keyPressedSignal(QKeyEvent* event);
    void doubleClickedSignal();
    void contextMenuSignal(const QPoint & pos);

public slots:
    void copyLineSlot();
    void copyTableSlot();
    void copyTableResizeSlot();
    void copyLineToLogSlot();
    void copyTableToLogSlot();
    void copyTableResizeToLogSlot();
    void copyEntrySlot();
    void contextMenuRequestedSlot(const QPoint & pos);
    void headerButtonPressedSlot(int col);

protected:
    QString copyTable(const std::vector<int> & colWidths);

    struct CellData
    {
        QString text;
        duint userdata = 0;
    };

    class ColumnCompare
    {
    public:
        ColumnCompare(int col, bool greater, SortBy::t fn)
        {
            mCol = col;
            mGreater = greater;
            mSortFn = fn;
        }

        inline bool operator()(const std::vector<CellData> & a, const std::vector<CellData> & b) const
        {
            bool less = mSortFn(a.at(mCol).text, b.at(mCol).text);
            if(mGreater)
                return !less;
            return less;
        }
    private:
        int mCol;
        int mGreater;
        SortBy::t mSortFn;
    };

    enum GuiState_t {NoState, MultiRowsSelectionState};

    typedef struct _SelectionData_t
    {
        int firstSelectedIndex;
        int fromIndex;
        int toIndex;
    } SelectionData_t;

    GuiState_t mGuiState;

    SelectionData_t mSelection;

    bool mIsMultiSelectionAllowed;
    bool mCopyMenuOnly;
    bool mCopyMenuDebugOnly;
    bool mIsColumnSortingAllowed;

    std::vector<std::vector<CellData>> mData; //listof(row) where row = (listof(col) where col = string)
    std::vector<SortBy::t> mColumnSortFunctions;
    std::vector<QString> mCopyTitles;
    QPair<int, bool> mSort;
};

#endif // STDTABLE_H
