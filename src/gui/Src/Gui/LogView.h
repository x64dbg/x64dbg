#ifndef LOGVIEW_H
#define LOGVIEW_H

#include <QTextEdit>
#include <cstdio>

class LogView : public QTextEdit
{
    Q_OBJECT
    Q_PROPERTY(int viewId MEMBER m_viewId)
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
    bool getLoggingEnabled();

    void clearLogSlot();
    void saveSlot();
    void toggleLoggingSlot();
private:
    int m_viewId;
    bool loggingEnabled;

    QAction* actionCopy;
    QAction* actionSelectAll;
    QAction* actionClear;
    QAction* actionSave;
    QAction* actionToggleLogging;
    QAction* actionRedirectLog;

    FILE* logRedirection;
};

#endif // LOGVIEW_H
