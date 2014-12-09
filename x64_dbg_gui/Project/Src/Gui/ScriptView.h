#ifndef SCRIPTVIEW_H
#define SCRIPTVIEW_H

#include "StdTable.h"

class ScriptView : public StdTable
{
    Q_OBJECT
public:
    explicit ScriptView(StdTable* parent = 0);
    void colorsUpdated();

    // Reimplemented Functions
    QString paintContent(QPainter* painter, int_t rowBase, int rowOffset, int col, int x, int y, int w, int h);
    void mouseDoubleClickEvent(QMouseEvent* event);
    void keyPressEvent(QKeyEvent* event);

public slots:
    void refreshShortcutsSlot();
    void contextMenuSlot(const QPoint & pos);
    void add(int count, const char** lines);
    void clear();
    void setIp(int line);
    void error(int line, QString message);
    void setTitle(QString title);
    void setInfoLine(int line, QString info);
    void openFile();
    void reload();
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
    QString filename;

    QMenu* mLoadMenu;
    QAction* mScriptLoad;
    QAction* mScriptReload;
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
