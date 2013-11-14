#ifndef WORDEDITDIALOG_H
#define WORDEDITDIALOG_H

#include <QtGui>
#include <QDialog>
#include <QPushButton>
#include "Bridge.h"

namespace Ui {
class WordEditDialog;
}

class WordEditDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit WordEditDialog(QWidget *parent = 0);
    ~WordEditDialog();
    void setup(QString title, uint_t defVal, int byteCount);
    uint_t getVal();

public slots:
    void expressionChanged(QString s);
    
private:
    Ui::WordEditDialog *ui;
    uint_t mWord;
};

#endif // WORDEDITDIALOG_H
