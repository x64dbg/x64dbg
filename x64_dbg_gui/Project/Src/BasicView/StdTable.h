#ifndef STDTABLE_H
#define STDTABLE_H

#include <QtGui>
#include "NewTypes.h"
#include "AbstractTableView.h"

class StdTable : public AbstractTableView
{
    Q_OBJECT
public:
    explicit StdTable(QWidget *parent = 0);
    QString paintContent(QPainter* painter, int_t rowBase, int rowOffset, int col, int x, int y, int w, int h);

    void mouseMoveEvent(QMouseEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);

    void enableMultiSelection(bool enabled);
    
    // Selection Management
    void expandSelectionUpTo(int to);
    void setSingleSelection(int index);
    int getInitialSelection();
    void selectNext();
    void selectPrevious();
    bool isSelected(int base, int offset);

    // Data Management
    void addColumnAt(int width, bool isClickable);
    void setRowCount(int count);
    void setCellContent(int r, int c, QString s);

signals:
    
public slots:

private:
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



    QList< QList<QString>* >* mData;
};

#endif // STDTABLE_H
