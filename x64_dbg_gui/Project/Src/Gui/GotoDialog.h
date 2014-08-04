#ifndef GOTODIALOG_H
#define GOTODIALOG_H

#include <QDialog>
#include <QPalette>
#include "Bridge.h"

namespace Ui
{
class GotoDialog;
}

class GotoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GotoDialog(QWidget* parent = 0);
    ~GotoDialog();
    QString expressionText;
    uint_t validRangeStart;
    uint_t validRangeEnd;

private slots:
    void on_editExpression_textChanged(const QString & arg1);
    void on_buttonOk_clicked();
    void finishedSlot(int result);

private:
    Ui::GotoDialog* ui;
    bool IsValidMemoryRange(uint_t addr);
};

#endif // GOTODIALOG_H
