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
    explicit GotoDialog(QWidget* parent = nullptr, bool allowInvalidExpression = false, bool allowInvalidAddress = false, bool allowNotDebugging = false);
    ~GotoDialog();
    int exec() override;
    void setInitialExpression(const QString & expression);

    QString expressionText;
    duint validRangeStart = 0;
    duint validRangeEnd = ~0;
    bool fileOffset = false;
    QString modName;
    bool allowInvalidExpression = false;
    bool allowInvalidAddress = false;
    bool allowNotDebugging = false;

protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private slots:
    void expressionChanged(bool validExpression, bool validPointer, dsint value);
    void disableAutoCompleteUpdated();
    void on_buttonOk_clicked();
    void finishedSlot(int result);
    void textEditedSlot(QString text);
    void linkActivated(const QString & link);

private:
    void validateExpression(const QString & expression);
    bool isValidMemoryRange(duint addr);
    void setOkEnabled(bool enabled);

    Ui::GotoDialog* ui = nullptr;
    ValidateExpressionThread* mValidateThread = nullptr;
    QCompleter* completer = nullptr;
    QString mCompletionText;
};
