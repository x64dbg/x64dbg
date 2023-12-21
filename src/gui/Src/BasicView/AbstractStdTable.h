#pragma once

#include "AbstractTableView.h"

class AbstractStdTable : public AbstractTableView
{
    Q_OBJECT
public:
    explicit AbstractStdTable(QWidget* parent = nullptr);
    QString paintContent(QPainter* painter, duint row, duint col, int x, int y, int w, int h) override;
    void updateColors() override;
    void reloadData() override;

    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

    void enableMultiSelection(bool enabled);
    void enableColumnSorting(bool enabled);

    // Selection Management
    void expandSelectionUpTo(duint to);
    void expandUp();
    void expandDown();
    void expandTop();
    void expandBottom();
    void setSingleSelection(duint index);
    duint getInitialSelection() const;
    QList<duint> getSelection() const;
    void selectStart();
    void selectEnd();
    void selectNext();
    void selectPrevious();
    void selectAll();
    bool isSelected(duint row) const;
    bool scrollSelect(duint row);

    // Data Management
    void addColumnAt(int width, QString title, bool isClickable, QString copyTitle = "");
    void deleteAllColumns() override;

    virtual QString getCellContent(duint row, duint column) = 0;
    virtual duint getCellUserdata(duint row, duint column) = 0;
    virtual bool isValidIndex(duint row, duint column) = 0;
    virtual void sortRows(duint column, bool ascending) = 0;

    //context menu helpers
    void setupCopyMenu(QMenu* copyMenu);
    void setupCopyColumnMenu(QMenu* copyMenu);
    void setupCopyMenu(MenuBuilder* copyMenu);
    void setupCopyColumnMenu(MenuBuilder* copyMenu);
    void setCopyMenuOnly(bool bSet, bool bDebugOnly = true);

    //draw helpers
    void setHighlightText(QString highlightText, duint minCol = 0)
    {
        mHighlightText = highlightText;
        mMinimumHighlightColumn = minCol;
    }

    void setAddressColumn(duint col, bool cipBase = false)
    {
        mAddressColumn = col;
        bCipBase = cipBase;
    }

    void setAddressLabel(bool addressLabel)
    {
        bAddressLabel = addressLabel;
    }

signals:
    void selectionChanged(duint index);
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
    void exportTableSlot();
    void contextMenuRequestedSlot(const QPoint & pos);
    void headerButtonPressedSlot(duint col);

protected:
    QString copyTable(const std::vector<int> & colWidths);
    duint getAddressForPosition(int x, int y) override;

    struct SelectionData
    {
        duint firstSelectedIndex = 0;
        duint fromIndex = 0;
        duint toIndex = 0;
    };

    SelectionData mSelection;

    enum GuiState
    {
        NoState,
        MultiRowsSelectionState
    } mGuiState = NoState;

    bool mIsMultiSelectionAllowed = false;
    bool mCopyMenuOnly = false;
    bool mCopyMenuDebugOnly = true;
    bool mIsColumnSortingAllowed = true;

    std::vector<QString> mCopyTitles;

    struct SortData
    {
        duint column = -1;
        bool ascending = true;
    } mSort;

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
    bool bCipBase = false;
    QString mHighlightText;
    int mMinimumHighlightColumn = 0;
    int mAddressColumn = -1;
    bool bAddressLabel = true;

    QAction* mCopyLine;
    QAction* mCopyTable;
    QAction* mCopyTableResize;
    QAction* mCopyLineToLog;
    QAction* mCopyTableToLog;
    QAction* mCopyTableResizeToLog;
    QAction* mExportTableCSV;
};
