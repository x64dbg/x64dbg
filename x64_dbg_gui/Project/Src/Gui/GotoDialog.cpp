#include "GotoDialog.h"
#include "ui_GotoDialog.h"

GotoDialog::GotoDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GotoDialog)
{
    //setup UI first
    ui->setupUi(this);
    setWindowFlags(Qt::MSWindowsFixedSizeDialogHint);
    setFixedSize(this->size()); //fixed size
    setModal(true); //modal window
    //initialize stuff
    if(!DbgIsDebugging()) //not debugging
        ui->labelError->setText("<font color='red'><b>Not debugging...</b></color>");
    else
        ui->labelError->setText("<font color='red'><b>Invalid expression...</b></color>");
    ui->buttonOk->setEnabled(false);
    ui->editExpression->setFocus();
}

GotoDialog::~GotoDialog()
{
    delete ui;
}

void GotoDialog::on_editExpression_textChanged(const QString &arg1)
{
    if(!DbgIsDebugging()) //not debugging
    {
        ui->labelError->setText("<font color='red'><b>Not debugging...</b></color>");
        ui->buttonOk->setEnabled(false);
        expressionText.clear();
    }
    else if(!Bridge::getBridge()->isValidExpression(arg1.toUtf8().constData())) //invalid expression
    {
        ui->labelError->setText("<font color='red'><b>Invalid expression...</b></color>");
        ui->buttonOk->setEnabled(false);
        expressionText.clear();
    }
    else
    {
        ui->labelError->setText("<font color='#00FF00'><b>Correct expression!</b></color>");
        ui->buttonOk->setEnabled(true);
        expressionText=arg1;
    }
}
