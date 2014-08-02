#include "CalculatorDialog.h"
#include "ui_CalculatorDialog.h"
#include <QString>

CalculatorDialog::CalculatorDialog(QWidget *parent) : QDialog(parent), ui(new Ui::CalculatorDialog)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog | Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::MSWindowsFixedSizeDialogHint);
    setFixedSize(this->size()); //fixed size
    connect(ui->txtExpression,SIGNAL(textChanged(QString)),this,SLOT(answerExpression(QString)));
    connect(this,SIGNAL(validAddress(bool)),ui->btnGoto,SLOT(setEnabled(bool)));
    emit validAddress(false);
    ui->txtExpression->setFocus();
}

CalculatorDialog::~CalculatorDialog()
{
    delete ui;
}

void CalculatorDialog::setExpressionFocus()
{
    ui->txtExpression->setFocus();
}

void CalculatorDialog::answerExpression(QString expression)
{

    if(!DbgIsValidExpression(expression.toUtf8().constData()))
    {
        ui->txtBin->setText("");
        ui->txtDec->setText("");
        ui->txtHex->setText("");
        ui->txtOct->setText("");
        ui->txtAscii->setText("");
        ui->txtUnicode->setText("");
    }
    else
    {
        uint_t ans = DbgValFromString(expression.toUtf8().constData());
        ui->txtHex->setText(inFormat(ans,N_HEX));
        ui->txtDec->setText(inFormat(ans,N_DEC));
        ui->txtBin->setText(inFormat(ans,N_BIN));
        ui->txtOct->setText(inFormat(ans,N_OCT));
        if((ans == (ans & 0xFF)) )
        {
            QChar c = QChar((char)ans);
            if(c.isPrint())
                ui->txtAscii->setText(QString(c));
            else
                ui->txtAscii->setText("???");
        }
        else
        {
            ui->txtAscii->setText("???");
            if((ans == (ans & 0xFFF)) ) //UNICODE?
            {
                QChar c = QChar((wchar_t)ans);
                if(c.isPrint())
                    ui->txtUnicode->setText(QString(c));
                else
                    ui->txtUnicode->setText("???");
            }
            else
            {
                ui->txtUnicode->setText("???");
            }
        }
        emit validAddress(DbgMemIsValidReadPtr(ans));
    }
}

QString CalculatorDialog::inFormat(const uint_t val, CalculatorDialog::NUMBERFORMAT NF) const
{
    switch(NF)
    {
    default:
    case N_HEX:
        // 0,...,9 in hex is the same as in dec
        if(val<10 && val>=0)
            return QString("%1").arg(val,1,16,QChar('0')).toUpper();
        return QString("%1").arg(val,1,16,QChar('0')).toUpper()+"h";
    case N_DEC:
        // 0,...,9 in hex is the same as in dec
        if(val<10 && val>=0)
            return QString("%1").arg(val);
        return QString("%1").arg(val);
    case N_BIN:
    {
        QString binary = QString("%1").arg(val,8*4,2,QChar('0')).toUpper();
        QString ans = "";
        for(int i=0; i<4*8; i++)
        {
            ans += binary[i];
            if((i%4==0) && (i!=0))
                ans += " ";
        }
        return ans +"b";
    }
    case N_OCT:
        return QString("%1").arg(val,1,8,QChar('0')).toUpper()+"o";
    case N_ASCII:
        return QString("'%1'").arg((char)val);
    }
}

void CalculatorDialog::on_btnGoto_clicked()
{
    DbgCmdExecDirect(QString("disasm " + ui->txtExpression->text()).toUtf8().constData());
    emit showCpu();
}
