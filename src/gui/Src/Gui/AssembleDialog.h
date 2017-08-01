#ifndef ASSEMBLEDIALOG_H
#define ASSEMBLEDIALOG_H

#include <QDialog>
#include <functional>
#include "Bridge.h"

class ValidateExpressionThread;

namespace Ui
{
    class AssembleDialog;
}

class AssembleDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AssembleDialog(QWidget* parent = 0);
    ~AssembleDialog();
    QString editText;
    static bool bWarningShowedOnce;
    void setTextEditValue(const QString & text);

    bool bKeepSizeChecked;
    void setKeepSizeChecked(bool checked);
    void setKeepSizeLabel(const QString & text);

    bool bFillWithNopsChecked;
    void setFillWithNopsChecked(bool checked);

    void setSelectedInstrVa(const duint va);
    void setOkButtonEnabled(bool enabled);

    void validateInstruction(QString expression);

private slots:
    void textChangedSlot(QString text);
    void instructionChangedSlot(dsint sizeDifference, QString error);
    void on_lineEdit_textChanged(const QString & arg1);
    void on_checkBoxKeepSize_clicked(bool checked);
    void on_checkBoxFillWithNops_clicked(bool checked);
    void on_radioXEDParse_clicked();
    void on_radioAsmjit_clicked();

private:
    Ui::AssembleDialog* ui;
    duint mSelectedInstrVa;
    ValidateExpressionThread* mValidateThread;
};

#endif // ASSEMBLEDIALOG_H
