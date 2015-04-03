#ifndef SOURCEVIEW_H
#define SOURCEVIEW_H

#include <QWidget>
#include "StdTable.h"

class SourceView : public StdTable
{
    Q_OBJECT
public:
    explicit SourceView(QString path, int line = 0, StdTable* parent = 0);
    QString getSourcePath();
    void setInstructionPointer(int line);
    QString paintContent(QPainter* painter, int_t rowBase, int rowOffset, int col, int x, int y, int w, int h);

private:
    QString mSourcePath;
    int mIpLine;
    void loadFile();
};

#endif // SOURCEVIEW_H
