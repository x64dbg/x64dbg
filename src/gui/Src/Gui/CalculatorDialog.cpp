#include "CalculatorDialog.h"
#include "ui_CalculatorDialog.h"
#include "ValidateExpressionThread.h"

static duint bswap(duint value)
{
    duint result = 0;
    for(size_t i = 0; i < sizeof(value); i++)
        ((unsigned char*)&result)[sizeof(value) - i - 1] = ((unsigned char*)&value)[i];
    return result;
}

QString CalculatorDialog::inFormat(const duint val, CalculatorDialog::Format format) const
{
    switch(format)
    {
    default:
    case Format::Hex:
        return QString("%1").arg(val, 1, 16, QChar('0')).toUpper();
    case Format::SignedDec:
        return QString("%1").arg((dsint)val);
    case Format::UnsignedDec:
        return QString("%1").arg(val);
    case Format::Binary:
    {
        QString binary = QString("%1").arg(val, 8 * sizeof(duint), 2, QChar('0')).toUpper();
        QString ans = "";
        for(int i = 0; i < sizeof(duint) * 8; i++)
        {
            if((i % 4 == 0) && (i != 0))
                ans += " ";
            ans += binary[i];
        }
        return ans;
    }
    case Format::Octal:
        return QString("%1").arg(val, 1, 8, QChar('0')).toUpper();
    case Format::Bytes:
        return QString("%1").arg(bswap(val), 2 * sizeof(duint), 16, QChar('0')).toUpper();
    }
}

CalculatorDialog::CalculatorDialog(QWidget* parent) : QDialog(parent), ui(new Ui::CalculatorDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint | Qt::MSWindowsFixedSizeDialogHint);

    connect(this, SIGNAL(validAddress(bool)), ui->btnGoto, SLOT(setEnabled(bool)));
    connect(this, SIGNAL(validAddress(bool)), ui->btnGotoDump, SLOT(setEnabled(bool)));
    connect(this, SIGNAL(validAddress(bool)), ui->btnGotoMemoryMap, SLOT(setEnabled(bool)));
    emit validAddress(false);

    ui->txtBytes->setInputMask(QString("HH").repeated(sizeof(duint)));
    ui->txtBin->setInputMask(QString("bbbb ").repeated(sizeof(duint) * 2).trimmed());

    ui->txtExpression->setText("0");
    ui->txtExpression->selectAll();
    ui->txtExpression->setFocus();

    mValidateThread = new ValidateExpressionThread(this);
    mValidateThread->setOnExpressionChangedCallback(std::bind(&CalculatorDialog::validateExpression, this, std::placeholders::_1));

    connect(mValidateThread, SIGNAL(expressionChanged(bool, bool, dsint)), this, SLOT(expressionChanged(bool, bool, dsint)));
    connect(ui->txtExpression, SIGNAL(textChanged(QString)), mValidateThread, SLOT(textChanged(QString)));
}

CalculatorDialog::~CalculatorDialog()
{
    mValidateThread->stop();
    mValidateThread->wait();
    delete ui;
}

void CalculatorDialog::validateExpression(QString expression)
{
    duint value;
    bool validExpression = DbgFunctions()->ValFromString(expression.toUtf8().constData(), &value);
    bool validPointer = validExpression && DbgMemIsValidReadPtr(value);
    this->mValidateThread->emitExpressionChanged(validExpression, validPointer, value);
}

void CalculatorDialog::setExpressionFocus()
{
    ui->txtExpression->selectAll();
    ui->txtExpression->setFocus();
}

void CalculatorDialog::showEvent(QShowEvent* event)
{
    Q_UNUSED(event);
    mValidateThread->start();
}

void CalculatorDialog::hideEvent(QHideEvent* event)
{
    Q_UNUSED(event);
    mValidateThread->stop();
    mValidateThread->wait();
}

void CalculatorDialog::expressionChanged(bool validExpression, bool validPointer, dsint value)
{
    if(!validExpression)
    {
        ui->txtHex->setText("");
        ui->txtSignedDec->setText("");
        ui->txtUnsignedDec->setText("");
        ui->txtOct->setText("");
        ui->txtBytes->setText("");
        ui->txtBin->setText("");
        ui->txtAscii->setText("");
        ui->txtUnicode->setText("");

        ui->txtExpression->setStyleSheet("border: 2px solid red");
        emit validAddress(false);
    }
    else
    {
        ui->txtExpression->setStyleSheet("");
        ui->txtHex->setText(inFormat(value, Format::Hex));
        ui->txtSignedDec->setText(inFormat(value, Format::SignedDec));
        ui->txtUnsignedDec->setText(inFormat(value, Format::UnsignedDec));
        ui->txtOct->setText(inFormat(value, Format::Octal));

        {
            int cursorpos = ui->txtBytes->cursorPosition();
            ui->txtBytes->setText(inFormat(value, Format::Bytes));
            ui->txtBytes->setCursorPosition(cursorpos);
        }

        {

            int cursorpos = ui->txtBin->cursorPosition();
            ui->txtBin->setText(inFormat(value, Format::Binary));
            ui->txtBin->setCursorPosition(cursorpos);
        }

        if(value == (value & 0xFF))
        {
            QChar c((ushort)value);
            if(c.isPrint())
                ui->txtAscii->setText(QString(c));
            else
                ui->txtAscii->setText("???");
        }
        else
            ui->txtAscii->setText("???");

        ui->txtAscii->setCursorPosition(0);
        ui->txtAscii->selectAll();
        if((value == (value & 0xFFFF))) //UNICODE?
        {
            QChar c = QChar((ushort)value);
            if(c.isPrint())
                ui->txtUnicode->setText(QString(c));
            else
                ui->txtUnicode->setText("????");
        }
        else
        {
            ui->txtUnicode->setText("????");
        }
        ui->txtUnicode->setCursorPosition(0);
        ui->txtUnicode->selectAll();

        emit validAddress(validPointer);
    }
}

void CalculatorDialog::on_txtExpression_textChanged(const QString & arg1)
{
    Q_UNUSED(arg1);
    ui->txtHex->setStyleSheet("");
    ui->txtSignedDec->setStyleSheet("");
    ui->txtUnsignedDec->setStyleSheet("");
    ui->txtOct->setStyleSheet("");
    ui->txtBytes->setStyleSheet("");
    ui->txtBin->setStyleSheet("");
    ui->txtAscii->setStyleSheet("");
    ui->txtUnicode->setStyleSheet("");
    emit validAddress(false);
}

void CalculatorDialog::on_txtHex_textEdited(const QString & arg1)
{
    bool ok = false;
    ULONGLONG val = arg1.toULongLong(&ok, 16);
    if(!ok)
    {
        ui->txtHex->setStyleSheet("border: 2px solid red");
        return;
    }
    ui->txtHex->setStyleSheet("");
    ui->txtExpression->setText(QString("%1").arg(val, 1, 16, QChar('0')).toUpper());
}

void CalculatorDialog::on_txtSignedDec_textEdited(const QString & arg1)
{
    bool ok = false;
    LONGLONG val = arg1.toLongLong(&ok, 10);
    if(!ok)
    {
        ui->txtUnsignedDec->setStyleSheet("border: 2px solid red");
        return;
    }
    ui->txtUnsignedDec->setStyleSheet("");
    ui->txtExpression->setText(QString("%1").arg(val, 1, 16, QChar('0')).toUpper());
}

void CalculatorDialog::on_txtUnsignedDec_textEdited(const QString & arg1)
{
    bool ok = false;
    LONGLONG val = arg1.toULongLong(&ok, 10);
    if(!ok)
    {
        ui->txtUnsignedDec->setStyleSheet("border: 2px solid red");
        return;
    }
    ui->txtUnsignedDec->setStyleSheet("");
    ui->txtExpression->setText(QString("%1").arg(val, 1, 16, QChar('0')).toUpper());
}

void CalculatorDialog::on_txtOct_textEdited(const QString & arg1)
{
    bool ok = false;
    ULONGLONG val = arg1.toULongLong(&ok, 8);
    if(!ok)
    {
        ui->txtOct->setStyleSheet("border: 2px solid red");
        return;
    }
    ui->txtOct->setStyleSheet("");
    ui->txtExpression->setText(QString("%1").arg(val, 1, 16, QChar('0')).toUpper());
}

void CalculatorDialog::on_txtBytes_textEdited(const QString & arg1)
{
    bool ok = false;
    QString text = arg1;
    text = text.leftJustified(sizeof(duint) * 2, '0', true);
    ULONGLONG val = text.toULongLong(&ok, 16);
    if(!ok)
    {
        ui->txtBytes->setStyleSheet("border: 2px solid red");
        return;
    }
    ui->txtBytes->setStyleSheet("");
    ui->txtExpression->setText(QString("%1").arg(bswap(val), 1, 16, QChar('0')).toUpper());
}

void CalculatorDialog::on_txtBin_textEdited(const QString & arg1)
{
    bool ok = false;
    QString text = arg1;
    text = text.replace(" ", "").leftJustified(sizeof(duint) * 8, '0', true);
    ULONGLONG val = text.toULongLong(&ok, 2);
    if(!ok)
    {
        ui->txtBin->setStyleSheet("border: 2px solid red");
        return;
    }
    ui->txtBin->setStyleSheet("");
    ui->txtExpression->setText(QString("%1").arg(val, 1, 16, QChar('0')).toUpper());
}

void CalculatorDialog::on_txtAscii_textEdited(const QString & arg1)
{
    QString text = arg1;
    ui->txtAscii->setStyleSheet("");
    ui->txtExpression->setText(QString().sprintf("%X", text[0].unicode()));
    ui->txtAscii->setCursorPosition(0);
    ui->txtAscii->selectAll();
}

void CalculatorDialog::on_txtUnicode_textEdited(const QString & arg1)
{
    QString text = arg1;
    ui->txtUnicode->setStyleSheet("");
    ui->txtExpression->setText(QString().sprintf("%X", text[0].unicode()));
    ui->txtUnicode->setCursorPosition(0);
    ui->txtUnicode->selectAll();
}

void CalculatorDialog::on_btnGoto_clicked()
{
    DbgCmdExecDirect(QString("disasm " + ui->txtHex->text()));
}

void CalculatorDialog::on_btnGotoDump_clicked()
{
    DbgCmdExecDirect(QString("dump " + ui->txtHex->text()));
}

void CalculatorDialog::on_btnGotoMemoryMap_clicked()
{
    DbgCmdExecDirect(QString("memmapdump " + ui->txtHex->text()));
}

