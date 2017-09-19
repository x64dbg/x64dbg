#ifndef TRACEBROWSER_H
#define TRACEBROWSER_H

#include "AbstractTableView.h"
#include "VaHistory.h"

class TraceFileReader;

class TraceBrowser : public AbstractTableView
{
    Q_OBJECT
public:
    explicit TraceBrowser(QWidget* parent = 0);
    QString paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h);

    void prepareData();

private:
    void setupRightClickContextMenu();
    void contextMenuEvent(QContextMenuEvent* event);

    VaHistory mHistory;
    MenuBuilder* mMenuBuilder;

    TraceFileReader* mTraceFile;

public slots:
    void openFileSlot();
    void parseFinishedSlot();
};

#endif //TRACEBROWSER_H
