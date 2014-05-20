#ifndef SCRIPTVIEW_H
#define SCRIPTVIEW_H

#include <QtGui>
#include <QAction>
#include <QMessageBox>
#include <QFileDialog>
#include <QMenu>
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
    void mouseDoubleClickEvent(QMouseEvent* event);
    void keyPressEvent(QKeyEvent* event);

public slots:
    void add(int count, const char** lines);
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
    void newIp();
    void question(QString message);
    void enableHighlighting(bool enable);

private:
    //private functions
    void setupContextMenu();
    void setSelection(int line);
    bool isScriptCommand(QString text, QString cmd);

    //private variables
    int mIpLine;
    bool mEnableSyntaxHighlighting;

    QMenu* mLoadMenu;
    QAction* mScriptLoad;
    QAction* mScriptUnload;
    QAction* mScriptRun;
    QAction* mScriptRunCursor;
    QAction* mScriptStep;
    QAction* mScriptBpToggle;
    QAction* mScriptCmdExec;
    QAction* mScriptAbort;
    QAction* mScriptNewIp;
};

#endif // SCRIPTVIEW_H
