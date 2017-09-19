#ifndef TRACEBROWSER_H
#define TRACEBROWSER_H

#include "AbstractTableView.h"
#include "VaHistory.h"

class TraceFileReader;
class QBeaEngine;

class TraceBrowser : public AbstractTableView
{
    Q_OBJECT
public:
    explicit TraceBrowser(QWidget* parent = 0);
    ~TraceBrowser();

    QString paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h);

    void prepareData();
    virtual void updateColors();

    void expandSelectionUpTo(duint to);
    void setSingleSelection(duint index);
    duint getInitialSelection();
    duint getSelectionSize();
    duint getSelectionStart();
    duint getSelectionEnd();

private:
    void setupRightClickContextMenu();
    void makeVisible(duint index);

    void contextMenuEvent(QContextMenuEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
    void keyPressEvent(QKeyEvent* event);

    VaHistory mHistory;
    MenuBuilder* mMenuBuilder;

    typedef struct _SelectionData_t
    {
        duint firstSelectedIndex;
        duint fromIndex;
        duint toIndex;
    } SelectionData_t;

    SelectionData_t mSelection;

    TraceFileReader* mTraceFile;
    QBeaEngine* mDisasm;

    QColor mBytesColor;
    QColor mBytesBackgroundColor;

public slots:

    void openFileSlot();
    void closeFileSlot();
    void parseFinishedSlot();

    void gotoSlot();
    void copyDisassemblySlot();
};

#endif //TRACEBROWSER_H
