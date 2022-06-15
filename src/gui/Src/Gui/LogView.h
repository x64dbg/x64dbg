#pragma once

#include <QTextBrowser>
#include <cstdio>
#include "LineEditDialog.h"

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
    void addMsgToLogSlot(QByteArray msg); /* Non-HTML Log Function*/
    void addMsgToLogSlotHtml(QByteArray msg); /* HTML accepting Log Function */
    void redirectLogSlot();
    void setLoggingEnabled(bool enabled);
    void autoScrollSlot();
    void findInLogSlot();
    void findNextInLogSlot();
    void findPreviousInLogSlot();
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

    void addMsgToLogSlotRaw(QByteArray msg, bool htmlEscape); /* Non-HTML Log Function*/

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
    QAction* actionFindInLog;
    QAction* actionFindNext;
    QAction* actionFindPrevious;
    LineEditDialog* dialogFindInLog;

    FILE* logRedirection;
    QString logBuffer;
    QTimer* flushTimer;
    bool flushLog;
    QString lastFindText;
};
