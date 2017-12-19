#pragma once

#include "AbstractTableView.h"

class AbstractStdTable : public AbstractTableView
{
    Q_OBJECT
public:
    explicit AbstractStdTable(QWidget* parent = 0);
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

    // Data Management
    void addColumnAt(int width, QString title, bool isClickable, QString copyTitle = "");
    void deleteAllColumns() override;

    virtual QString getCellContent(int r, int c) = 0;
    virtual bool isValidIndex(int r, int c) = 0;
    virtual void sortRows(int column, bool ascending) = 0;

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

    struct
    {
        int firstSelectedIndex = 0;
        int fromIndex = 0;
        int toIndex = 0;
    } mSelection;

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
};
