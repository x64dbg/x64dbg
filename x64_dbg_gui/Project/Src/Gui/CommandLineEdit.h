#ifndef LINEEDIT_H
#define LINEEDIT_H

#include <QtGui>
#include <QDebug>
#include <QLineEdit>

class CommandLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit CommandLineEdit(QWidget *parent = 0);
    void keyPressEvent(QKeyEvent* event);
    void addCmdToHistory(QString parCmd);
    void setFocusToCmd();
    
signals:
    void keyPressed(int parKey);
    
public slots:


private:
    QList<QString> mCmdHistory;
    int mCmdIndex;
    
};

#endif // LINEEDIT_H
