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

    enum class Format
    {
        Unknown = 0,
        Hex,
        SignedDec,
        UnsignedDec,
        Octal,
        Bytes,
        Binary,

    };

    QString inFormat(const duint val, CalculatorDialog::Format format) const;

public:
    explicit CalculatorDialog(QWidget* parent = nullptr);
    ~CalculatorDialog();
    void validateExpression(QString expression);
    void setExpressionFocus();
    void showEvent(QShowEvent* event);
    void hideEvent(QHideEvent* event);

signals:
    bool validAddress(bool valid);

private slots:
    void expressionChanged(bool validExpression, bool validPointer, dsint value);
    void on_txtExpression_textChanged(const QString & arg1);
    void on_txtHex_textEdited(const QString & arg1);
    void on_txtSignedDec_textEdited(const QString & arg1);
    void on_txtUnsignedDec_textEdited(const QString & arg1);
    void on_txtOct_textEdited(const QString & arg1);
    void on_txtBytes_textEdited(const QString & arg1);
    void on_txtBin_textEdited(const QString & arg1);
    void on_txtAscii_textEdited(const QString & arg1);
    void on_txtUnicode_textEdited(const QString & arg1);
    void on_btnGoto_clicked();
    void on_btnGotoDump_clicked();
    void on_btnGotoMemoryMap_clicked();

private:
    ValidateExpressionThread* mValidateThread;
    Ui::CalculatorDialog* ui;
};
