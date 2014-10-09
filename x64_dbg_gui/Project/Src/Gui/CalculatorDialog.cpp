#include "CalculatorDialog.h"
#include "ui_CalculatorDialog.h"

CalculatorDialog::CalculatorDialog(QWidget* parent) : QDialog(parent), ui(new Ui::CalculatorDialog)
{
    ui->setupUi(this);
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
    setWindowFlags(Qt::Dialog | Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::MSWindowsFixedSizeDialogHint);
#endif
    setFixedSize(this->size()); //fixed size
    connect(this, SIGNAL(validAddress(bool)), ui->btnGoto, SLOT(setEnabled(bool)));
    emit validAddress(false);
    ui->txtBin->setInputMask(QString("bbbb ").repeated(sizeof(uint_t) * 2));
    ui->txtExpression->setText("0");
    ui->txtExpression->selectAll();
    ui->txtExpression->setFocus();
    mValidateThread = new CalculatorDialogValidateThread(this);
}

CalculatorDialog::~CalculatorDialog()
{
    delete ui;
}

void CalculatorDialog::showEvent(QShowEvent* event)
{
    Q_UNUSED(event);
    mValidateThread->start();
}

void CalculatorDialog::hideEvent(QHideEvent* event)
{
    Q_UNUSED(event);
    mValidateThread->terminate();
}

void CalculatorDialog::setExpressionFocus()
{
    ui->txtExpression->selectAll();
    ui->txtExpression->setFocus();
}

void CalculatorDialog::validateExpression()
{
    QString expression = ui->txtExpression->text();
    if(expressionText == expression)
        return;
    expressionText = expression;
    if(!DbgIsValidExpression(expression.toUtf8().constData()))
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
        uint_t ans = DbgValFromString(expression.toUtf8().constData());
        ui->txtHex->setText(inFormat(ans, N_HEX));
        ui->txtSignedDec->setText(inFormat(ans, N_SDEC));
        ui->txtUnsignedDec->setText(inFormat(ans, N_UDEC));
        int cursorpos = ui->txtBin->cursorPosition();
        ui->txtBin->setText(inFormat(ans, N_BIN));
        ui->txtBin->setCursorPosition(cursorpos);
        ui->txtOct->setText(inFormat(ans, N_OCT));
        if((ans == (ans & 0xFF)))
        {
            QChar c = QChar::fromLatin1((char)ans);
            if(c.isPrint())
                ui->txtAscii->setText("'" + QString(c) + "'");
            else
                ui->txtAscii->setText("???");
        }
        else
            ui->txtAscii->setText("???");
        ui->txtAscii->setCursorPosition(1);
        if((ans == (ans & 0xFFF)))  //UNICODE?
        {
            QChar c = QChar::fromLatin1((wchar_t)ans);
            if(c.isPrint())
                ui->txtUnicode->setText("L'" + QString(c) + "'");
            else
                ui->txtUnicode->setText("????");
        }
        else
        {
            ui->txtUnicode->setText("????");
        }
        ui->txtUnicode->setCursorPosition(2);
        emit validAddress(DbgMemIsValidReadPtr(ans));
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

QString CalculatorDialog::inFormat(const uint_t val, CalculatorDialog::NUMBERFORMAT NF) const
{
    switch(NF)
    {
    default:
    case N_HEX:
        return QString("%1").arg(val, 1, 16, QChar('0')).toUpper();
    case N_SDEC:
        return QString("%1").arg((int_t)val);
    case N_UDEC:
        return QString("%1").arg(val);
    case N_BIN:
    {
        QString binary = QString("%1").arg(val, 8 * sizeof(uint_t), 2, QChar('0')).toUpper();
        QString ans = "";
        for(int i = 0; i < sizeof(uint_t) * 8; i++)
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
        return QString("'%1'").arg((char)val);
    }
}

void CalculatorDialog::on_btnGoto_clicked()
{
    DbgCmdExecDirect(QString("disasm " + ui->txtExpression->text()).toUtf8().constData());
    emit showCpu();
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
    text = text.replace("'", "");
    if(text.length() > 1)
    {
        ui->txtAscii->setStyleSheet("border: 2px solid red");
        return;
    }
    ui->txtAscii->setStyleSheet("");
    ui->txtExpression->setText(QString().sprintf("%X", text[0].toLatin1()));
    ui->txtAscii->setCursorPosition(1);
}

void CalculatorDialog::on_txtUnicode_textEdited(const QString & arg1)
{
    QString text = arg1;
    text = text.replace("L'", "").replace("'", "");
    if(text.length() > 1)
    {
        ui->txtUnicode->setStyleSheet("border: 2px solid red");
        return;
    }
    ui->txtUnicode->setStyleSheet("");
    ui->txtExpression->setText(QString().sprintf("%X", text[0].unicode()));
    ui->txtUnicode->setCursorPosition(2);
}
