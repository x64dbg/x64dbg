#pragma once

#include <QDialog>
#include "Imports.h"

class ValidateExpressionThread;
class QCompleter;

namespace Ui
{
    class GotoDialog;
}

class GotoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GotoDialog(QWidget* parent = 0, bool allowInvalidExpression = false, bool allowInvalidAddress = false, bool allowNotDebugging = false);
    ~GotoDialog();
    QString expressionText;
    duint validRangeStart;
    duint validRangeEnd;
    bool fileOffset;
    QString modName;
    bool allowInvalidExpression;
    bool allowInvalidAddress;
    bool allowNotDebugging;
    void showEvent(QShowEvent* event);
    void hideEvent(QHideEvent* event);
    void validateExpression(QString expression);
    void setInitialExpression(const QString & expression);

private slots:
    void expressionChanged(bool validExpression, bool validPointer, dsint value);
    void disableAutoCompleteUpdated();
    void on_buttonOk_clicked();
    void finishedSlot(int result);
    void textEditedSlot(QString text);
    void linkActivated(const QString & link);

private:
    Ui::GotoDialog* ui;
    ValidateExpressionThread* mValidateThread;
    QCompleter* completer;
    bool IsValidMemoryRange(duint addr);
    void setOkEnabled(bool enabled);
    QString mCompletionText;
};
