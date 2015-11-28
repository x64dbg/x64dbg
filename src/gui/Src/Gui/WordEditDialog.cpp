#include "WordEditDialog.h"
#include "ui_WordEditDialog.h"

WordEditDialog::WordEditDialog(QWidget* parent) : QDialog(parent), ui(new Ui::WordEditDialog)
{
    ui->setupUi(this);
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
    setWindowFlags(Qt::Dialog | Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::MSWindowsFixedSizeDialogHint);
#endif
    setModal(true);

    // Set up default validators for numerical input
    ui->signedLineEdit->setValidator(new QRegExpValidator(QRegExp("^-?\\d*(\\d+)?$"), this));// Optional negative, 0-9
    ui->unsignedLineEdit->setValidator(new QRegExpValidator(QRegExp("^\\d*(\\d+)?$"), this));// No signs, 0-9

    mValidateThread = new ValidateExpressionThread(this);
    connect(mValidateThread, SIGNAL(expressionChanged(bool, bool, dsint)), this, SLOT(expressionChanged(bool, bool, dsint)));
    connect(ui->expressionLineEdit, SIGNAL(textChanged(QString)), mValidateThread, SLOT(textChanged(QString)));
    mWord = 0;
}

WordEditDialog::~WordEditDialog()
{
    delete ui;
}

void WordEditDialog::showEvent(QShowEvent* event)
{
    Q_UNUSED(event);
    mValidateThread->start(ui->expressionLineEdit->text());
}

void WordEditDialog::hideEvent(QHideEvent* event)
{
    Q_UNUSED(event);
    mValidateThread->stop();
    mValidateThread->wait();
}

void WordEditDialog::setup(QString title, duint defVal, int byteCount)
{
    this->setWindowTitle(title);
    ui->hexLineEdit->setInputMask(QString("hh").repeated(byteCount));
    ui->expressionLineEdit->setText(QString("%1").arg(defVal, byteCount * 2, 16, QChar('0')).toUpper());

    ui->expressionLineEdit->selectAll();
    ui->expressionLineEdit->setFocus();
}

duint WordEditDialog::getVal()
{
    return mWord;
}

void WordEditDialog::expressionChanged(bool validExpression, bool validPointer, dsint value)
{
    Q_UNUSED(validPointer);
    if(validExpression)
    {
        ui->expressionLineEdit->setStyleSheet("");
        ui->signedLineEdit->setStyleSheet("");
        ui->unsignedLineEdit->setStyleSheet("");
        ui->buttons->button(QDialogButtonBox::Ok)->setEnabled(true);

        //hex
        mWord = value;
        duint hexWord = 0;
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
        ui->hexLineEdit->setText(QString("%1").arg(hexWord, sizeof(duint) * 2, 16, QChar('0')).toUpper());
        //signed
        ui->signedLineEdit->setText(QString::number((dsint)mWord));
        //unsigned
        ui->unsignedLineEdit->setText(QString::number((duint)mWord));
    }
    else
    {
        ui->expressionLineEdit->setStyleSheet("border: 1px solid red");
        ui->buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
}

void WordEditDialog::on_signedLineEdit_textEdited(const QString & arg1)
{
    LONGLONG value;
    if(sscanf_s(arg1.toUtf8().constData(), "%lld", &value) == 1)
    {
        ui->signedLineEdit->setStyleSheet("");
        ui->buttons->button(QDialogButtonBox::Ok)->setEnabled(true);
        ui->expressionLineEdit->setText(QString("%1").arg((duint)value, sizeof(duint) * 2, 16, QChar('0')).toUpper());
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
        ui->expressionLineEdit->setText(QString("%1").arg((duint)value, sizeof(duint) * 2, 16, QChar('0')).toUpper());
    }
    else
    {
        ui->unsignedLineEdit->setStyleSheet("border: 1px solid red");
        ui->buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
}
