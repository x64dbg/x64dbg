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
    bool getLoggingEnabled();
    void onAnchorClicked(const QUrl & link);

    void clearLogSlot();
    void saveSlot();
    void toggleLoggingSlot();
private:
    bool loggingEnabled;
    bool autoScroll;

    QAction* actionCopy;
    QAction* actionSelectAll;
    QAction* actionClear;
    QAction* actionSave;
    QAction* actionToggleLogging;
    QAction* actionRedirectLog;
    QAction* actionAutoScroll;

    FILE* logRedirection;
};

#endif // LOGVIEW_H
