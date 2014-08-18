#ifndef LINEEDITDIALOG_H
#define LINEEDITDIALOG_H

namespace Ui
{
class LineEditDialog;
}

class LineEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LineEditDialog(QWidget* parent = 0);
    ~LineEditDialog();
    QString editText;
    bool bChecked;
    void setText(const QString & text);
    void enableCheckBox(bool bEnable);
    void setCheckBox(bool bSet);
    void setCheckBoxText(const QString & text);

private slots:
    void on_textEdit_textChanged(const QString & arg1);
    void on_checkBox_toggled(bool checked);

private:
    Ui::LineEditDialog* ui;
};

#endif // LINEEDITDIALOG_H
