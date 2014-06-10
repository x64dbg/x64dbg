#ifndef LINEEDITDIALOG_H
#define LINEEDITDIALOG_H

#include <QDialog>

namespace Ui
{
class LineEditDialog;
}

class LineEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LineEditDialog(QWidget *parent = 0);
    ~LineEditDialog();
    QString editText;
    void setText(const QString &text);

private slots:
    void on_textEdit_textChanged(const QString &arg1);

private:
    Ui::LineEditDialog *ui;
};

#endif // LINEEDITDIALOG_H
