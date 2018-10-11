#include "SimpleFilterDialog.h"
#include "ui_SimpleFilterDialog.h"

SimpleFilterDialog::SimpleFilterDialog(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::SimpleFilterDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint | Qt::MSWindowsFixedSizeDialogHint);

    mMnemonicFilterText = "";
    minVA = 0;
    maxVA = std::numeric_limits<duint>::max();

    ui->lineEdit_mnemonic->setFocus();
}

SimpleFilterDialog::~SimpleFilterDialog()
{
    delete ui;
}

void SimpleFilterDialog::initInputWidgets()
{
    ui->lineEdit_mnemonic->setText(mMnemonicFilterText);
    if(maxVA < std::numeric_limits<duint>::max())
    {
        ui->lineEdit_max_va->setText(QString::number(maxVA, 16));
    }

    if(minVA > 0)
    {
        ui->lineEdit_min_va->setText(QString::number(minVA, 16));
    }
}

void SimpleFilterDialog::on_lineEdit_mnemonic_textChanged(const QString & arg1)
{
    mMnemonicFilterText = arg1;
}

void SimpleFilterDialog::on_lineEdit_max_va_textEdited(const QString & arg1)
{
    bool success = false;
    duint va = arg1.toULongLong(&success, 16);

    if(success
            && arg1.length() <= 16
            && arg1.length() > 0)
    {
        mPreviousMaxVaText = arg1;
        maxVA = va;
    }
    else if(arg1.length() <= 0)
    {
        mPreviousMaxVaText = "";
        maxVA = std::numeric_limits<quint64>::max();
    }
    else
    {
        ui->lineEdit_max_va->setText(mPreviousMaxVaText);
    }
}

void SimpleFilterDialog::on_lineEdit_min_va_textEdited(const QString & arg1)
{
    bool success = false;
    duint va = arg1.toULongLong(&success, 16);

    if(success
            && arg1.length() <= 16
            && arg1.length() > 0)
    {
        mPreviousMinVaText = arg1;
        minVA = va;
    }
    else if(arg1.length() <= 0)
    {
        mPreviousMinVaText = "";
        minVA = 0;
    }
    else
    {
        ui->lineEdit_min_va->setText(mPreviousMinVaText);
    }
}
