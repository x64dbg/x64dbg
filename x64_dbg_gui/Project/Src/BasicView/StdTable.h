#ifndef STDTABLE_H
#define STDTABLE_H

#include "AbstractTableView.h"

class StdTable : public AbstractTableView
{
    Q_OBJECT
public:
    explicit StdTable(QWidget* parent = 0);
    QString paintContent(QPainter* painter, int_t rowBase, int rowOffset, int col, int x, int y, int w, int h);
    void reloadData();

    void mouseMoveEvent(QMouseEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void mouseDoubleClickEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
    void keyPressEvent(QKeyEvent* event);

    void enableMultiSelection(bool enabled);

    // Selection Management
    void expandSelectionUpTo(int to);
    void setSingleSelection(int index);
    int getInitialSelection();
    void selectNext();
    void selectPrevious();
    bool isSelected(int base, int offset);

    // Data Management
    void addColumnAt(int width, QString title, bool isClickable, QString copyTitle = "");
    void setRowCount(int count);
    void deleteAllColumns();
    void setCellContent(int r, int c, QString s);
    QString getCellContent(int r, int c);
    bool isValidIndex(int r, int c);

    //context menu helpers
    void setupCopyMenu(QMenu* copyMenu);
    void setCopyMenuOnly(bool bSet, bool bDebugOnly = true);

signals:
    void selectionChangedSignal(int index);
    void keyPressedSignal(QKeyEvent* event);
    void doubleClickedSignal();
    void contextMenuSignal(const QPoint & pos);

public slots:
    void copyLineSlot();
    void copyTableSlot();
    void copyEntrySlot();
    void contextMenuRequestedSlot(const QPoint & pos);
    void headerButtonPressedSlot(int col);

private:
    class ColumnCompare
    {
    public:
        ColumnCompare(int col, bool greater)
        {
            mCol = col;
            mGreater = greater;
        }

        inline bool operator()(const QList<QString> & a, const QList<QString> & b) const
        {
            bool less = QString::compare(a.at(mCol), b.at(mCol), Qt::CaseInsensitive) < 0;
            if(mGreater)
                return !less;
            return less;
        }
    private:
        int mCol;
        int mGreater;
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

    bool mIsMultiSelctionAllowed;
    bool mCopyMenuOnly;
    bool mCopyMenuDebugOnly;

    QList<QList<QString>> mData;
    QList<QString> mCopyTitles;
    QPair<int, bool> mSort;
};

#endif // STDTABLE_H
