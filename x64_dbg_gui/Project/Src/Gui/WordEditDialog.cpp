#include "WordEditDialog.h"
#include "ui_WordEditDialog.h"

WordEditDialog::WordEditDialog(QWidget* parent) : QDialog(parent), ui(new Ui::WordEditDialog)
{
    ui->setupUi(this);
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
    setWindowFlags(Qt::Dialog | Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::MSWindowsFixedSizeDialogHint);
#endif
    setModal(true);

    mValidateThread = new WordEditDialogValidateThread(this);
    mWord = 0;
}

WordEditDialog::~WordEditDialog()
{
    delete ui;
}

void WordEditDialog::showEvent(QShowEvent* event)
{
    Q_UNUSED(event);
    mValidateThread->start();
}

void WordEditDialog::hideEvent(QHideEvent* event)
{
    Q_UNUSED(event);
    mValidateThread->terminate();
}

void WordEditDialog::setup(QString title, uint_t defVal, int byteCount)
{
    this->setWindowTitle(title);
    ui->hexLineEdit->setInputMask(QString("hh").repeated(byteCount));
    ui->expressionLineEdit->setText(QString("%1").arg(defVal, byteCount * 2, 16, QChar('0')).toUpper());

    ui->expressionLineEdit->selectAll();
    ui->expressionLineEdit->setFocus();
}

uint_t WordEditDialog::getVal()
{
    return mWord;
}

void WordEditDialog::validateExpression()
{
    QString expression = ui->expressionLineEdit->text();
    if(expressionText == expression)
        return;
    expressionText = expression;
    if(DbgIsValidExpression(expression.toUtf8().constData()))
    {
        ui->expressionLineEdit->setStyleSheet("");
        ui->unsignedLineEdit->setStyleSheet("");
        ui->signedLineEdit->setStyleSheet("");
        ui->buttons->button(QDialogButtonBox::Ok)->setEnabled(true);

        //hex
        mWord = DbgValFromString(expression.toUtf8().constData());
        uint_t hexWord = 0;
        unsigned char* hex = (unsigned char*)&hexWord;
        unsigned char* word = (unsigned char*)&mWord;
#ifdef _WIN64
        hex[0] = word[7];
        hex[1] = word[6];
        hex[2] = word[5];
        hex[3] = word[4];
        hex[4] = word[3];
        hex[5] = word[2];
        hex[6] = word[1];
        hex[7] = word[0];
#else //x86
        hex[0] = word[3];
        hex[1] = word[2];
        hex[2] = word[1];
        hex[3] = word[0];
#endif //_WIN64
        ui->hexLineEdit->setText(QString("%1").arg(hexWord, sizeof(uint_t) * 2, 16, QChar('0')).toUpper());
        //signed
        ui->signedLineEdit->setText(QString::number((int_t)mWord));
        //unsigned
        ui->unsignedLineEdit->setText(QString::number((uint_t)mWord));
    }
    else
    {
        ui->expressionLineEdit->setStyleSheet("border: 1px solid red");
        ui->buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
}

void WordEditDialog::on_expressionLineEdit_textChanged(const QString & arg1)
{
    Q_UNUSED(arg1);
    ui->buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
}

void WordEditDialog::on_signedLineEdit_textEdited(const QString & arg1)
{
    LONGLONG value;
    if(sscanf_s(arg1.toUtf8().constData(), "%lld", &value) == 1)
    {
        ui->signedLineEdit->setStyleSheet("");
        ui->buttons->button(QDialogButtonBox::Ok)->setEnabled(true);
        ui->expressionLineEdit->setText(QString("%1").arg((uint_t)value, sizeof(uint_t) * 2, 16, QChar('0')).toUpper());
    }
    else
    {
        ui->signedLineEdit->setStyleSheet("border: 1px solid red");
        ui->buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
}

void WordEditDialog::on_unsignedLineEdit_textEdited(const QString & arg1)
{
    LONGLONG value;
    if(sscanf_s(arg1.toUtf8().constData(), "%llu", &value) == 1)
    {
        ui->unsignedLineEdit->setStyleSheet("");
        ui->buttons->button(QDialogButtonBox::Ok)->setEnabled(true);
        ui->expressionLineEdit->setText(QString("%1").arg((uint_t)value, sizeof(uint_t) * 2, 16, QChar('0')).toUpper());
    }
    else
    {
        ui->unsignedLineEdit->setStyleSheet("border: 1px solid red");
        ui->buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
}
