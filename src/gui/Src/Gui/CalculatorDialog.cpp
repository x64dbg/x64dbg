#include "CalculatorDialog.h"
#include "ui_CalculatorDialog.h"
#include "ValidateExpressionThread.h"

CalculatorDialog::CalculatorDialog(QWidget* parent) : QDialog(parent), ui(new Ui::CalculatorDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint | Qt::MSWindowsFixedSizeDialogHint);
    connect(this, SIGNAL(validAddress(bool)), ui->btnGoto, SLOT(setEnabled(bool)));
    connect(this, SIGNAL(validAddress(bool)), ui->btnGotoDump, SLOT(setEnabled(bool)));
    emit validAddress(false);
    ui->txtBin->setInputMask(QString("bbbb ").repeated(sizeof(duint) * 2));
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

void CalculatorDialog::setExpressionFocus()
{
    ui->txtExpression->selectAll();
    ui->txtExpression->setFocus();
}

void CalculatorDialog::expressionChanged(bool validExpression, bool validPointer, dsint value)
{
    if(!validExpression)
    {
        ui->txtBin->setText("");
        ui->txtSignedDec->setText("");
        ui->txtUnsignedDec->setText("");
        ui->txtHex->setText("");
        ui->txtOct->setText("");
        ui->txtAscii->setText("");
        ui->txtUnicode->setText("");
        ui->txtExpression->setStyleSheet("border: 2px solid red");
        emit validAddress(false);
    }
    else
    {
        ui->txtExpression->setStyleSheet("");
        ui->txtHex->setText(inFormat(value, N_HEX));
        ui->txtSignedDec->setText(inFormat(value, N_SDEC));
        ui->txtUnsignedDec->setText(inFormat(value, N_UDEC));
        int cursorpos = ui->txtBin->cursorPosition();
        ui->txtBin->setText(inFormat(value, N_BIN));
        ui->txtBin->setCursorPosition(cursorpos);
        ui->txtOct->setText(inFormat(value, N_OCT));
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
    ui->txtBin->setStyleSheet("");
    ui->txtAscii->setStyleSheet("");
    ui->txtUnicode->setStyleSheet("");
    emit validAddress(false);
}

QString CalculatorDialog::inFormat(const duint val, CalculatorDialog::NUMBERFORMAT NF) const
{
    switch(NF)
    {
    default:
    case N_HEX:
        return QString("%1").arg(val, 1, 16, QChar('0')).toUpper();
    case N_SDEC:
        return QString("%1").arg((dsint)val);
    case N_UDEC:
        return QString("%1").arg(val);
    case N_BIN:
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
    case N_OCT:
        return QString("%1").arg(val, 1, 8, QChar('0')).toUpper();
    case N_ASCII:
        return QString("%1").arg(QChar((ushort)val));
    }
}

void CalculatorDialog::on_btnGoto_clicked()
{
    DbgCmdExecDirect(QString("disasm " + ui->txtExpression->text()));
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

void CalculatorDialog::on_txtBin_textEdited(const QString & arg1)
{
    bool ok = false;
    QString text = arg1;
    text = text.replace(" ", "");
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

void CalculatorDialog::on_txtAscii_clicked()
{
    ui->txtAscii->selectAll();
}

void CalculatorDialog::on_txtUnicode_clicked()
{
    ui->txtUnicode->selectAll();
}

void CalculatorDialog::on_btnGotoDump_clicked()
{
    DbgCmdExecDirect(QString("dump " + ui->txtExpression->text()));
}
