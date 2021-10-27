#pragma once

#include <QDialog>

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
    void setPlaceholderText(const QString & text);
    void setTextMaxLength(int length);
    void enableCheckBox(bool bEnable);
    void setCheckBox(bool bSet);
    void setCheckBoxText(const QString & text);
    void setCursorPosition(int position);
    void ForceSize(unsigned int size);
    void setFpuMode();
    void selectAllText();

private slots:
    void on_textEdit_textEdited(const QString & arg1);
    void on_checkBox_toggled(bool checked);
    void on_buttonOk_clicked();
    void on_buttonCancel_clicked();

private:
    Ui::LineEditDialog* ui;
    unsigned int fixed_size;
    bool fpuMode;
};
