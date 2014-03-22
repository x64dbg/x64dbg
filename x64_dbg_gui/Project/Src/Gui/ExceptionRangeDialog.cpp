#include "ExceptionRangeDialog.h"
#include "ui_ExceptionRangeDialog.h"

ExceptionRangeDialog::ExceptionRangeDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ExceptionRangeDialog)
{
    ui->setupUi(this);
    //set window flags
    setModal(true);
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::MSWindowsFixedSizeDialogHint);
    setFixedSize(this->size()); //fixed size
    ui->editStart->setCursorPosition(0);
    ui->editEnd->setCursorPosition(0);
    ui->btnOk->setEnabled(false);
}

ExceptionRangeDialog::~ExceptionRangeDialog()
{
    delete ui;
}

void ExceptionRangeDialog::on_editStart_textChanged(const QString &arg1)
{
    if(!ui->editStart->text().size()) //nothing entered
    {
        ui->btnOk->setEnabled(false);
        return;
    }
    bool converted=false;
    unsigned long start=ui->editStart->text().toUInt(&converted, 16);
    if(!converted)
    {
        ui->btnOk->setEnabled(false);
        return;
    }
    unsigned long end=ui->editEnd->text().toUInt(&converted, 16);
    if(converted && end<start)
        ui->btnOk->setEnabled(false);
    else
        ui->btnOk->setEnabled(true);
}


void ExceptionRangeDialog::on_editEnd_textChanged(const QString &arg1)
{
    if(!ui->editEnd->text().size() || !ui->editStart->text().size())
    {
        ui->btnOk->setEnabled(false);
        return;
    }
    bool converted=false;
    unsigned long start=ui->editStart->text().toUInt(&converted, 16);
    if(!converted)
    {
        ui->btnOk->setEnabled(false);
        return;
    }
    unsigned long end=ui->editEnd->text().toUInt(&converted, 16);
    if(!converted)
    {
        ui->btnOk->setEnabled(false);
        return;
    }
    if(end<start)
        ui->btnOk->setEnabled(false);
    else
        ui->btnOk->setEnabled(true);
}

void ExceptionRangeDialog::on_btnOk_clicked()
{
    rangeStart=ui->editStart->text().toUInt(0, 16);
    bool converted=false;
    rangeEnd=ui->editEnd->text().toUInt(&converted, 16);
    if(!converted)
        rangeEnd=rangeStart;
    accept();
}
