#include "LineEditDialog.h"
#include "ui_LineEditDialog.h"

LineEditDialog::LineEditDialog(QWidget* parent) : QDialog(parent), ui(new Ui::LineEditDialog)
{
    ui->setupUi(this);
    setModal(true);
    setWindowFlags(Qt::Dialog | Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::MSWindowsFixedSizeDialogHint);
    setFixedSize(this->size()); //fixed size
    setModal(true); //modal window
    ui->checkBox->hide();
    bChecked = false;
}

LineEditDialog::~LineEditDialog()
{
    delete ui;
}

void LineEditDialog::setText(const QString & text)
{
    ui->textEdit->setText(text);
    ui->textEdit->selectAll();
}

void LineEditDialog::enableCheckBox(bool bEnable)
{
    if(bEnable)
        ui->checkBox->show();
    else
        ui->checkBox->hide();
}

void LineEditDialog::setCheckBox(bool bSet)
{
    ui->checkBox->setChecked(bSet);
    bChecked = bSet;
}

void LineEditDialog::setCheckBoxText(const QString & text)
{
    ui->checkBox->setText(text);
}

void LineEditDialog::on_textEdit_textChanged(const QString & arg1)
{
    editText = arg1;
}

void LineEditDialog::on_checkBox_toggled(bool checked)
{
    bChecked = checked;
}
