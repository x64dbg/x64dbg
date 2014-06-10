#ifndef HISTORYLINEEDIT_H
#define HISTORYLINEEDIT_H

#include <QtGui>
#include <QDebug>
#include <QLineEdit>

class HistoryLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit HistoryLineEdit(QWidget *parent = 0);
    void keyPressEvent(QKeyEvent* event);
    void addLineToHistory(QString parLine);
    void setFocus();

signals:
    void keyPressed(int parKey);

public slots:


private:
    QList<QString> mCmdHistory;
    int mCmdIndex;

};

#endif // HISTORYLINEEDIT_H
