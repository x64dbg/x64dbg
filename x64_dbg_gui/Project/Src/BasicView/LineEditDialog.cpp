#include "LineEditDialog.h"
#include "ui_LineEditDialog.h"

LineEditDialog::LineEditDialog(QWidget *parent) : QDialog(parent), ui(new Ui::LineEditDialog)
{
    ui->setupUi(this);
    setModal(true);
    setWindowFlags(Qt::Dialog | Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::MSWindowsFixedSizeDialogHint);
    setFixedSize(this->size()); //fixed size
    setModal(true); //modal window
}

LineEditDialog::~LineEditDialog()
{
    delete ui;
}

void LineEditDialog::setText(const QString &text)
{
    ui->textEdit->setText(text);
    ui->textEdit->selectAll();
}

void LineEditDialog::on_textEdit_textChanged(const QString &arg1)
{
    editText=arg1;
}
