#include "WordEditDialog.h"
#include "ui_WordEditDialog.h"

WordEditDialog::WordEditDialog(QWidget *parent) : QDialog(parent), ui(new Ui::WordEditDialog)
{
    ui->setupUi(this);

    setModal(true);
    setWindowFlags(Qt::Dialog | Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::MSWindowsFixedSizeDialogHint);


    mWord = 0;

    connect(ui->expressionLineEdit, SIGNAL(textChanged(const QString)), this, SLOT(expressionChanged(QString)));
}

WordEditDialog::~WordEditDialog()
{
    delete ui;
}

void WordEditDialog::expressionChanged(QString s)
{
    bool wIsValid = Bridge::getBridge()->isValidExpression(s.toUtf8().constData());

    if(wIsValid == true)
    {
        ui->expressionLineEdit->setStyleSheet("");
        ui->buttons->button(QDialogButtonBox::Ok)->setEnabled(true);
        mWord = Bridge::getBridge()->getValFromString(reinterpret_cast<const char*>(s.toUtf8().constData()));
        ui->hexLineEdit->setText(QString("%1").arg(mWord, sizeof(uint_t) * 2, 16, QChar('0')).toUpper());
        ui->signedLineEdit->setText(QString::number((int_t)mWord));
        ui->unsignedLineEdit->setText(QString::number((uint_t)mWord));
    }
    else
    {
        ui->expressionLineEdit->setStyleSheet("border: 1px solid red");
        ui->buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
}



void WordEditDialog::setup(QString title, uint_t defVal, int byteCount)
{
    this->setWindowTitle(title);
    ui->expressionLineEdit->setText(QString("%1").arg(defVal, byteCount * 2, 16, QChar('0')).toUpper());
    ui->hexLineEdit->setText(QString("%1").arg(defVal, byteCount * 2, 16, QChar('0')).toUpper());
    ui->signedLineEdit->setText(QString::number((int_t)defVal));
    ui->unsignedLineEdit->setText(QString::number((uint_t)defVal));
    ui->hexLineEdit->setInputMask(QString("hh ").repeated(byteCount));
    ui->expressionLineEdit->selectAll();
    ui->expressionLineEdit->setFocus();
}


uint_t WordEditDialog::getVal()
{
    return mWord;
}
