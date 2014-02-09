#ifndef SCRIPTVIEW_H
#define SCRIPTVIEW_H

#include <QtGui>
#include "StdTable.h"
#include "Bridge.h"

class ScriptView : public StdTable
{
    Q_OBJECT
public:
    explicit ScriptView(StdTable *parent = 0);
    // Reimplemented Functions
    QString paintContent(QPainter* painter, int_t rowBase, int rowOffset, int col, int x, int y, int w, int h);

public slots:
    void addLine(QString text);
    void clear();
    void setIp(int line);
    void error(int line, QString message);
    void setTitle(QString title);
    void setInfoLine(int line, QString info);

private:
    int mIpLine;
};

#endif // SCRIPTVIEW_H
