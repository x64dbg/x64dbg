#ifndef LOGVIEW_H
#define LOGVIEW_H

#include <QTextBrowser>
#include <cstdio>

class LogView : public QTextBrowser
{
    Q_OBJECT
public:
    explicit LogView(QWidget* parent = 0);
    ~LogView();
    void setupContextMenu();
    void contextMenuEvent(QContextMenuEvent* event);

public slots:
    void refreshShortcutsSlot();
    void updateStyle();
    void addMsgToLogSlot(QString msg);
    void redirectLogSlot();
    void setLoggingEnabled(bool enabled);
    void autoScrollSlot();
    void copyToGlobalNotes();
    void copyToDebuggeeNotes();
    void pasteSlot();
    bool getLoggingEnabled();
    void onAnchorClicked(const QUrl & link);

    void clearLogSlot();
    void saveSlot();
    void toggleLoggingSlot();
private:
    bool loggingEnabled;
    bool autoScroll;

    QAction* actionCopy;
    QAction* actionPaste;
    QAction* actionSelectAll;
    QAction* actionClear;
    QAction* actionSave;
    QAction* actionToggleLogging;
    QAction* actionRedirectLog;
    QAction* actionAutoScroll;
    QMenu* menuCopyToNotes;
    QAction* actionCopyToGlobalNotes;
    QAction* actionCopyToDebuggeeNotes;

    FILE* logRedirection;
};

#endif // LOGVIEW_H
