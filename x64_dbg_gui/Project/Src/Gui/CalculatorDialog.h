#ifndef CALCULATORDIALOG_H
#define CALCULATORDIALOG_H

#include <QDialog>
#include <QThread>
#include "NewTypes.h"

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
    void setExpressionFocus();
    void validateExpression();
    void showEvent(QShowEvent* event);
    void hideEvent(QHideEvent* event);

signals:
    bool validAddress(bool valid);
    void showCpu();

private slots:
    void on_btnGoto_clicked();
    void on_txtHex_textEdited(const QString & arg1);
    void on_txtSignedDec_textEdited(const QString & arg1);
    void on_txtUnsignedDec_textEdited(const QString & arg1);
    void on_txtOct_textEdited(const QString & arg1);
    void on_txtBin_textEdited(const QString & arg1);
    void on_txtAscii_textEdited(const QString & arg1);
    void on_txtUnicode_textEdited(const QString & arg1);
    void on_txtExpression_textChanged(const QString & arg1);

private:
    QString expressionText;
    QThread* mValidateThread;
    Ui::CalculatorDialog* ui;
    QString inFormat(const uint_t val, CalculatorDialog::NUMBERFORMAT NF) const;
};

class CalculatorDialogValidateThread : public QThread
{
    Q_OBJECT
public:
    CalculatorDialogValidateThread(CalculatorDialog* calculatorDialog)
    {
        mCalculatorDialog = calculatorDialog;
    }

private:
    CalculatorDialog* mCalculatorDialog;

    void run()
    {
        while(true)
        {
            mCalculatorDialog->validateExpression();
            Sleep(50);
        }
    }
};

#endif // CALCULATORDIALOG_H
