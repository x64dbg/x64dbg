#ifndef GOTODIALOG_H
#define GOTODIALOG_H

#include <QDialog>
#include <QPalette>
#include "Bridge.h"

namespace Ui {
class GotoDialog;
}

class GotoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GotoDialog(QWidget *parent = 0);
    ~GotoDialog();
    QString expressionText;

private slots:
    void on_editExpression_textChanged(const QString &arg1);

private:
    Ui::GotoDialog *ui;
};

#endif // GOTODIALOG_H
