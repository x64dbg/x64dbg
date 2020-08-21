#pragma once

#include "AbstractTableView.h"

class AbstractStdTable : public AbstractTableView
{
    Q_OBJECT
public:
    explicit AbstractStdTable(QWidget* parent = 0);
    QString paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h) override;
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
    void expandSelectionUpTo(int to);
    void expandUp();
    void expandDown();
    void expandTop();
    void expandBottom();
    void setSingleSelection(int index);
    int getInitialSelection() const;
    QList<int> getSelection() const;
    void selectStart();
    void selectEnd();
    void selectNext();
    void selectPrevious();
    void selectAll();
    bool isSelected(int base, int offset) const;
    bool scrollSelect(int offset);

    // Data Management
    void addColumnAt(int width, QString title, bool isClickable, QString copyTitle = "");
    void deleteAllColumns() override;

    virtual QString getCellContent(int r, int c) = 0;
    virtual bool isValidIndex(int r, int c) = 0;
    virtual void sortRows(int column, bool ascending) = 0;
    duint getDisassemblyPopupAddress(int mousex, int mousey) override;

    //context menu helpers
    void setupCopyMenu(QMenu* copyMenu);
    void setupCopyColumnMenu(QMenu* copyMenu);
    void setupCopyMenu(MenuBuilder* copyMenu);
    void setupCopyColumnMenu(MenuBuilder* copyMenu);
    void setCopyMenuOnly(bool bSet, bool bDebugOnly = true);

    //draw helpers
    void setHighlightText(QString highlightText)
    {
        mHighlightText = highlightText;
    }

    void setAddressColumn(int col, bool cipBase = false)
    {
        mAddressColumn = col;
        bCipBase = cipBase;
    }

    void setAddressLabel(bool addressLabel)
    {
        bAddressLabel = addressLabel;
    }

    bool setDisassemblyPopupEnabled(bool enabled)
    {
        return bDisassemblyPopupEnabled = enabled;
    }

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
    void exportTableSlot();
    void contextMenuRequestedSlot(const QPoint & pos);
    void headerButtonPressedSlot(int col);

protected:
    QString copyTable(const std::vector<int> & colWidths);

    struct SelectionData
    {
        int firstSelectedIndex = 0;
        int fromIndex = 0;
        int toIndex = 0;
    };

    SelectionData mSelection;

    enum
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
        int column = -1;
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
    int mAddressColumn = -1;
    bool bAddressLabel = true;
    bool bDisassemblyPopupEnabled = true;

    QAction* mCopyLine;
    QAction* mCopyTable;
    QAction* mCopyTableResize;
    QAction* mCopyLineToLog;
    QAction* mCopyTableToLog;
    QAction* mCopyTableResizeToLog;
    QAction* mExportTableCSV;
};
