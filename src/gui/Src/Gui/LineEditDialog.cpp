#include "LineEditDialog.h"
#include "ui_LineEditDialog.h"

LineEditDialog::LineEditDialog(QWidget* parent) : QDialog(parent), ui(new Ui::LineEditDialog)
{
    ui->setupUi(this);
    setModal(true);
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
    setWindowFlags(Qt::Dialog | Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::MSWindowsFixedSizeDialogHint);
#endif
    setFixedSize(this->size()); //fixed size
    setModal(true); //modal window
    ui->checkBox->hide();
    bChecked = false;
    this->fixed_size = 0;
}

LineEditDialog::~LineEditDialog()
{
    delete ui;
}

void LineEditDialog::setCursorPosition(int position)
{
    ui->textEdit->setCursorPosition(position);
}

void LineEditDialog::ForceSize(unsigned int size)
{
    this->fixed_size = size;

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
    if(this->fixed_size != 0)
    {
        if(arg1.size() != this->fixed_size)
        {
            ui->buttonOk->setEnabled(false);
            QString symbolct = "";
            int ct = arg1.size() - (int) this->fixed_size;
            if(ct > 0)
                symbolct = "+";
            ui->label->setText(QString("<font color='red'>") + QString("CT: ") + symbolct + QString::number(ct) + QString("</font>"));
        }
        else
        {
            ui->buttonOk->setEnabled(true);
            ui->label->setText(QString(""));
        }
    }
}

void LineEditDialog::on_checkBox_toggled(bool checked)
{
    bChecked = checked;
}
