#ifndef WORDEDITDIALOG_H
#define WORDEDITDIALOG_H

#include <QDialog>
#include <QThread>
#include <QPushButton>
#include "NewTypes.h"

namespace Ui
{
class WordEditDialog;
}

class WordEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit WordEditDialog(QWidget* parent = 0);
    ~WordEditDialog();
    void setup(QString title, uint_t defVal, int byteCount);
    uint_t getVal();
    void showEvent(QShowEvent* event);
    void hideEvent(QHideEvent* event);
    void validateExpression();

private slots:
    void on_expressionLineEdit_textChanged(const QString & arg1);
    void on_signedLineEdit_textEdited(const QString & arg1);
    void on_unsignedLineEdit_textEdited(const QString & arg1);

private:
    Ui::WordEditDialog* ui;
    uint_t mWord;
    QString expressionText;
    QThread* mValidateThread;
};

class WordEditDialogValidateThread : public QThread
{
    Q_OBJECT
public:
    WordEditDialogValidateThread(WordEditDialog* wordEditDialog)
    {
        mWordEditDialog = wordEditDialog;
    }

private:
    WordEditDialog* mWordEditDialog;

    void run()
    {
        while(true)
        {
            mWordEditDialog->validateExpression();
            Sleep(50);
        }
    }
};

#endif // WORDEDITDIALOG_H
