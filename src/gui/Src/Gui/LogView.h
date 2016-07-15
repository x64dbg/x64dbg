#ifndef LOGVIEW_H
#define LOGVIEW_H

#include <QTextEdit>
#include <windows.h>

class LogView : public QTextEdit
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
    bool getLoggingEnabled();

    void clearLogSlot();
    void saveSlot();
    void toggleLoggingSlot();
private:
    bool loggingEnabled;

    QAction* actionCopy;
    QAction* actionSelectAll;
    QAction* actionClear;
    QAction* actionSave;
    QAction* actionToggleLogging;
    QAction* actionRedirectLog;

    HANDLE logRedirection;
};

#endif // LOGVIEW_H
