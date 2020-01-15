#include "LineEditDialog.h"
#include "ui_LineEditDialog.h"

LineEditDialog::LineEditDialog(QWidget* parent) : QDialog(parent), ui(new Ui::LineEditDialog)
{
    ui->setupUi(this);
    setModal(true);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint | Qt::MSWindowsFixedSizeDialogHint);
    setModal(true); //modal window
    ui->checkBox->hide();
    bChecked = false;
    this->fixed_size = 0;
    fpuMode = false;
    ui->label->setVisible(false);
}

LineEditDialog::~LineEditDialog()
{
    delete ui;
}

void LineEditDialog::selectAllText()
{
    ui->textEdit->selectAll();
}

void LineEditDialog::setCursorPosition(int position)
{
    ui->textEdit->setCursorPosition(position);
}

void LineEditDialog::ForceSize(unsigned int size)
{
    this->fixed_size = size;
    if(this->fixed_size)
        ui->label->setVisible(true);
}

void LineEditDialog::setFpuMode()
{
    fpuMode = true;
}

void LineEditDialog::setText(const QString & text)
{
    editText = text;
    ui->textEdit->setText(text);
    ui->textEdit->selectAll();
}

void LineEditDialog::setPlaceholderText(const QString & text)
{
    ui->textEdit->setPlaceholderText(text);
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

void LineEditDialog::on_textEdit_textEdited(const QString & arg1)
{
    editText = arg1;
    if(this->fixed_size != 0)
    {
        QString arg1Lower = arg1.toLower();
        if(arg1.size() != this->fixed_size && (!fpuMode || !(arg1.contains(QChar('.')) || arg1Lower == "inf" || arg1Lower == "nan" || arg1Lower == "+inf" || arg1Lower == "-inf")))
            //TODO: QNaN & SNaN
        {
            ui->buttonOk->setEnabled(false);
            QString symbolct = "";
            int ct = arg1.size() - (int) this->fixed_size;
            if(ct > 0)
                symbolct = "+";
            ui->label->setText(tr("<font color='red'>CT: %1%2</font>").arg(symbolct).arg(ct));
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

void LineEditDialog::setTextMaxLength(int length)
{
    ui->textEdit->setMaxLength(length);
}

void LineEditDialog::on_buttonOk_clicked()
{
    ui->textEdit->addLineToHistory(editText);
    ui->textEdit->setText("");
    accept();
}

void LineEditDialog::on_buttonCancel_clicked()
{
    ui->textEdit->setText("");
    close();
}
