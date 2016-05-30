#ifndef LOGVIEW_H
#define LOGVIEW_H

#include <QTextEdit>

class LogView : public QTextEdit
{
    Q_OBJECT
public:
    explicit LogView(QWidget* parent = 0);
    void setupContextMenu();
    void contextMenuEvent(QContextMenuEvent* event);

public slots:
    void refreshShortcutsSlot();
    void updateStyle();
    void addMsgToLogSlot(QString msg);
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
};

#endif // LOGVIEW_H
