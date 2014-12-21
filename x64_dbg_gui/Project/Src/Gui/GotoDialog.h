#ifndef GOTODIALOG_H
#define GOTODIALOG_H

#include <QDialog>
#include <QThread>
#include "NewTypes.h"

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
    bool fileOffset;
    QString modName;
    void showEvent(QShowEvent* event);
    void hideEvent(QHideEvent* event);
    void validateExpression();

private slots:
    void on_editExpression_textChanged(const QString & arg1);
    void on_buttonOk_clicked();
    void finishedSlot(int result);

private:
    Ui::GotoDialog* ui;
    QThread* mValidateThread;
    bool IsValidMemoryRange(uint_t addr);
};

class GotoDialogValidateThread : public QThread
{
    Q_OBJECT
public:
    GotoDialogValidateThread(GotoDialog* gotoDialog)
    {
        mGotoDialog = gotoDialog;
    }

private:
    GotoDialog* mGotoDialog;

    void run()
    {
        while(true)
        {
            mGotoDialog->validateExpression();
            Sleep(50);
        }
    }
};

#endif // GOTODIALOG_H
