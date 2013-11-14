#ifndef DUMP_H
#define DUMP_H

#include <QtGui>
#include <QtDebug>
#include "NewTypes.h"
#include "AbstractTableView.h"
#include "MemoryPage.h"
#include "QBeaEngine.h"
#include "Bridge.h"

class HexDump : public AbstractTableView
{
    Q_OBJECT
public:
    explicit HexDump(QWidget *parent = 0);

    //QString getStringToPrint(int rowBase, int rowOffset, int col);
    void mouseMoveEvent(QMouseEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);

    QString paintContent(QPainter* painter, int_t rowBase, int rowOffset, int col, int x, int y, int w, int h);
    void paintGraphicDump(QPainter* painter, int x, int y, int addr);

    // Selection Management
    void expandSelectionUpTo(int to);
    void setSingleSelection(int index);
    int getInitialSelection();
    bool isSelected(int base, int offset);


signals:
    
public slots:
    void printDumpAt(int_t parVA);

private:
    enum GuiState_t {NoState, MultiRowsSelectionState};

    typedef struct _SelectionData_t
    {
        int firstSelectedIndex;
        int fromIndex;
        int toIndex;
    } SelectionData_t;

    SelectionData_t mSelection;
    
    GuiState_t mGuiState;

    int mByteWidth;

    int mDumpByteWidth;

    MemoryPage* mMemPage;
};

#endif // DUMP_H
