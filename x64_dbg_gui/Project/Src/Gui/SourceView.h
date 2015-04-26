#ifndef SOURCEVIEW_H
#define SOURCEVIEW_H

#include <QWidget>
#include <QMenu>
#include <QAction>
#include "StdTable.h"

class SourceView : public StdTable
{
    Q_OBJECT
public:
    explicit SourceView(QString path, int line = 0, StdTable* parent = 0);
    QString getSourcePath();
    void setInstructionPointer(int line);
    QString paintContent(QPainter* painter, int_t rowBase, int rowOffset, int col, int x, int y, int w, int h);
    void setupContextMenu();
    void setSelection(int line);

signals:
    void showCpu();

public slots:
    void contextMenuSlot(const QPoint & pos);
    void followInDisasmSlot();

private:
    QAction* mFollowInDisasm;

    QString mSourcePath;
    int mIpLine;
    void loadFile();
};

#endif // SOURCEVIEW_H
