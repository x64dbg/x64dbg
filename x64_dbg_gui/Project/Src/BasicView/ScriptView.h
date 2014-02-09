#ifndef SCRIPTVIEW_H
#define SCRIPTVIEW_H

#include <QtGui>
#include "StdTable.h"
#include "Bridge.h"
#include "LineEditDialog.h"

class ScriptView : public StdTable
{
    Q_OBJECT
public:
    explicit ScriptView(StdTable *parent = 0);

    // Reimplemented Functions
    QString paintContent(QPainter* painter, int_t rowBase, int rowOffset, int col, int x, int y, int w, int h);
    void contextMenuEvent(QContextMenuEvent* event);

public slots:
    void addLine(QString text);
    void clear();
    void setIp(int line);
    void error(int line, QString message);
    void setTitle(QString title);
    void setInfoLine(int line, QString info);
    void openFile();
    void unload();
    void run();
    void bpToggle();
    void runCursor();
    void step();
    void abort();
    void cmdExec();
    void message(QString message);

private:
    //private functions
    void setupContextMenu();

    //private variables
    int mIpLine;

    QMenu* mLoadMenu;
    QAction* mScriptLoad;
    QAction* mScriptUnload;
    QAction* mScriptRun;
    QAction* mScriptRunCursor;
    QAction* mScriptStep;
    QAction* mScriptBpToggle;
    QAction* mScriptCmdExec;
    QAction* mScriptAbort;
};

#endif // SCRIPTVIEW_H
