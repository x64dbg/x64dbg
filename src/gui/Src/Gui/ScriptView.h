#pragma once

#include "StdTable.h"

class QMessageBox;
class MRUList;
class LineEditDialog;

class ScriptView : public StdTable
{
    Q_OBJECT
public:
    explicit ScriptView(StdTable* parent = nullptr);

    // Configuration
    void updateColors() override;

    // Reimplemented Functions
    QString paintContent(QPainter* painter, duint row, duint col, int x, int y, int w, int h) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

public slots:
    void contextMenuSlot(const QPoint & pos);
    void add(int count, const char** lines);
    void clear();
    void setIp(int line);
    void error(int line, QString message);
    void setTitle(QString title);
    void setInfoLine(int line, QString info);
    void openRecentFile(QString file);
    void openFile();
    void paste();
    void reload();
    void unload();
    void edit();
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
    void messageResult(int result);
    void shutdownSlot();

private:
    //private functions
    void setupContextMenu();
    void setSelection(int line);
    bool isScriptCommand(QString text, QString cmd, QString & mnemonic, QString & argument);

    //private variables
    int mIpLine;
    bool mEnableSyntaxHighlighting;
    QString filename;

    MenuBuilder* mMenu;
    QMessageBox* msg;
    MRUList* mMRUList;
    LineEditDialog* mCmdLineEdit;
};
