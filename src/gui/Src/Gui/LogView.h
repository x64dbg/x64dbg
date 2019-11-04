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
    void contextMenuEvent(QContextMenuEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

public slots:
    void refreshShortcutsSlot();
    void updateStyle();
    void addMsgToLogSlot(QByteArray msg);
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
    void flushTimerSlot();
    void flushLogSlot();

private:
    static const int MAX_LOG_BUFFER_SIZE = 1024 * 1024;

    bool loggingEnabled;
    bool autoScroll;
    bool utf16Redirect = false;

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
    QString logBuffer;
    QTimer* flushTimer;
    bool flushLog;
};

#endif // LOGVIEW_H
