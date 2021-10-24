#pragma once

#include <QDialog>
#include "Imports.h"

class ValidateExpressionThread;

namespace Ui
{
    class CalculatorDialog;
}

class CalculatorDialog : public QDialog
{
    Q_OBJECT

    enum NUMBERFORMAT
    {
        N_HEX = 16,
        N_SDEC = 10,
        N_UDEC = 11,
        N_BIN = 2,
        N_OCT = 8,
        N_ASCII = 0,
        N_UNKNOWN = -1
    };

public:
    explicit CalculatorDialog(QWidget* parent = 0);
    ~CalculatorDialog();
    void validateExpression(QString expression);
    void setExpressionFocus();
    void showEvent(QShowEvent* event);
    void hideEvent(QHideEvent* event);

signals:
    bool validAddress(bool valid);

private slots:
    void expressionChanged(bool validExpression, bool validPointer, dsint value);
    void on_btnGoto_clicked();
    void on_txtHex_textEdited(const QString & arg1);
    void on_txtSignedDec_textEdited(const QString & arg1);
    void on_txtUnsignedDec_textEdited(const QString & arg1);
    void on_txtOct_textEdited(const QString & arg1);
    void on_txtBin_textEdited(const QString & arg1);
    void on_txtAscii_textEdited(const QString & arg1);
    void on_txtUnicode_textEdited(const QString & arg1);
    void on_txtExpression_textChanged(const QString & arg1);

    void on_btnGotoDump_clicked();

private:
    ValidateExpressionThread* mValidateThread;
    Ui::CalculatorDialog* ui;
    QString inFormat(const duint val, CalculatorDialog::NUMBERFORMAT NF) const;
};
