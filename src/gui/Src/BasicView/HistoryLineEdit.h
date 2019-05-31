#ifndef HISTORYLINEEDIT_H
#define HISTORYLINEEDIT_H

#include <QLineEdit>
#include <QKeyEvent>

class HistoryLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit HistoryLineEdit(QWidget* parent = 0);
    void keyPressEvent(QKeyEvent* event);
    void addLineToHistory(QString parLine);
    QString getLineFromHistory();
    QString addHistoryClear();
    void setFocus();
    void loadSettings(QString sectionPrefix);
    void saveSettings(QString sectionPrefix);

signals:
    void keyPressed(int parKey);

private:
    int mCmdHistoryMaxSize = 1000;
    QList<QString> mCmdHistory;
    int mCmdIndex;
    bool bSixPressed;

};

#endif // HISTORYLINEEDIT_H
