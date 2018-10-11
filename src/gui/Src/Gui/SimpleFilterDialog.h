#pragma once

#include <QDialog>

#include "dbg_types.h"

namespace Ui
{
    class SimpleFilterDialog;
}

class SimpleFilterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SimpleFilterDialog(QWidget* parent = nullptr);
    ~SimpleFilterDialog();

    void initInputWidgets();

    QString mMnemonicFilterText;
    duint maxVA;
    duint minVA;

private slots:
    void on_lineEdit_mnemonic_textChanged(const QString & arg1);
    void on_lineEdit_max_va_textEdited(const QString & arg1);
    void on_lineEdit_min_va_textEdited(const QString & arg1);

private:
    Ui::SimpleFilterDialog* ui;

    QString mPreviousMaxVaText;
    QString mPreviousMinVaText;
};
