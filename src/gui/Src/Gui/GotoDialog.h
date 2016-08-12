#ifndef GOTODIALOG_H
#define GOTODIALOG_H

#include <QDialog>
#include "Imports.h"

class ValidateExpressionThread;

namespace Ui
{
    class GotoDialog;
}

class GotoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GotoDialog(QWidget* parent = 0, bool allowInvalidExpression = false, bool allowInvalidAddress = false);
    ~GotoDialog();
    QString expressionText;
    duint validRangeStart;
    duint validRangeEnd;
    bool fileOffset;
    QString modName;
    bool allowInvalidExpression;
    bool allowInvalidAddress;
    void showEvent(QShowEvent* event);
    void hideEvent(QHideEvent* event);
    void validateExpression(QString expression);
    void setInitialExpression(const QString & expression);

private slots:
    void expressionChanged(bool validExpression, bool validPointer, dsint value);
    void on_buttonOk_clicked();
    void finishedSlot(int result);

private:
    Ui::GotoDialog* ui;
    ValidateExpressionThread* mValidateThread;
    bool IsValidMemoryRange(duint addr);
    void setOkEnabled(bool enabled);
};

#endif // GOTODIALOG_H
