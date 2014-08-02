#ifndef CALCULATORDIALOG_H
#define CALCULATORDIALOG_H

#include <QDialog>
#include "Bridge.h"
namespace Ui {
class CalculatorDialog;
}

class CalculatorDialog : public QDialog
{
    Q_OBJECT

    enum NUMBERFORMAT{N_HEX=16,N_DEC=10,N_BIN=2,N_OCT=8,N_ASCII=0,N_UNKNOWN=-1};
public:
    explicit CalculatorDialog(QWidget *parent = 0);
    ~CalculatorDialog();
signals:
    bool validAddress(bool valid);
public slots:
    void answerExpression(QString expression);
private:
    Ui::CalculatorDialog *ui;
    QString inFormat(const uint_t val, CalculatorDialog::NUMBERFORMAT NF) const;
};

#endif // CALCULATORDIALOG_H
