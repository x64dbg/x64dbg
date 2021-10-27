#pragma once

#include <QDialog>

namespace Ui
{
    class ExceptionRangeDialog;
}

class ExceptionRangeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExceptionRangeDialog(QWidget* parent = 0);
    ~ExceptionRangeDialog();

    unsigned long rangeStart;
    unsigned long rangeEnd;

private slots:
    void on_editStart_textChanged(const QString & arg1);
    void on_editEnd_textChanged(const QString & arg1);
    void on_btnOk_clicked();

private:
    Ui::ExceptionRangeDialog* ui;
};
