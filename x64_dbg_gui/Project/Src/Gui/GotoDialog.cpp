#include "GotoDialog.h"
#include "ui_GotoDialog.h"

GotoDialog::GotoDialog(QWidget* parent) : QDialog(parent), ui(new Ui::GotoDialog)
{
    //setup UI first
    ui->setupUi(this);
    setModal(true);
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
    setWindowFlags(Qt::Dialog | Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::MSWindowsFixedSizeDialogHint);
#endif
    setFixedSize(this->size()); //fixed size
    //initialize stuff
    if(!DbgIsDebugging()) //not debugging
        ui->labelError->setText("<font color='red'><b>Not debugging...</b></font>");
    else
        ui->labelError->setText("<font color='red'><b>Invalid expression...</b></font>");
    ui->buttonOk->setEnabled(false);
    ui->editExpression->setFocus();
    validRangeStart = 0;
    validRangeEnd = 0;
    fileOffset = false;
    mValidateThread = new GotoDialogValidateThread(this);
    connect(this, SIGNAL(finished(int)), this, SLOT(finishedSlot(int)));
}

GotoDialog::~GotoDialog()
{
    delete ui;
}

void GotoDialog::showEvent(QShowEvent* event)
{
    Q_UNUSED(event);
    mValidateThread->start();
}

void GotoDialog::hideEvent(QHideEvent* event)
{
    Q_UNUSED(event);
    mValidateThread->terminate();
}

void GotoDialog::validateExpression()
{
    QString expression = ui->editExpression->text();
    if(expressionText == expression)
        return;
    if(!DbgIsDebugging()) //not debugging
    {
        ui->labelError->setText("<font color='red'><b>Not debugging...</b></font>");
        ui->buttonOk->setEnabled(false);
        expressionText.clear();
    }
    else if(!DbgIsValidExpression(expression.toUtf8().constData())) //invalid expression
    {
        ui->labelError->setText("<font color='red'><b>Invalid expression...</b></font>");
        ui->buttonOk->setEnabled(false);
        expressionText.clear();
    }
    else if(fileOffset)
    {
        uint_t offset = DbgValFromString(expression.toUtf8().constData());
        uint_t va = DbgFunctions()->FileOffsetToVa(modName.toUtf8().constData(), offset);
        if(va)
        {
            QString addrText = QString("%1").arg(va, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
            ui->labelError->setText(QString("<font color='#00DD00'><b>Correct expression! -> </b></font>" + addrText));
            ui->buttonOk->setEnabled(true);
            expressionText = expression;
        }
        else
        {
            ui->labelError->setText("<font color='red'><b>Invalid file offset...</b></font>");
            ui->buttonOk->setEnabled(false);
            expressionText.clear();
        }
    }
    else
    {
        uint_t addr = DbgValFromString(expression.toUtf8().constData());
        if(!DbgMemIsValidReadPtr(addr))
        {
            ui->labelError->setText("<font color='red'><b>Invalid memory address...</b></font>");
            ui->buttonOk->setEnabled(false);
            expressionText.clear();
        }
        else if(!IsValidMemoryRange(addr))
        {
            ui->labelError->setText("<font color='red'><b>Memory out of range...</b></font>");
            ui->buttonOk->setEnabled(false);
            expressionText.clear();
        }
        else
        {
            QString addrText;
            char module[MAX_MODULE_SIZE] = "";
            char label[MAX_LABEL_SIZE] = "";
            if(DbgGetLabelAt(addr, SEG_DEFAULT, label)) //has label
            {
                if(DbgGetModuleAt(addr, module) && !QString(label).startsWith("JMP.&"))
                    addrText = QString(module) + "." + QString(label);
                else
                    addrText = QString(label);
            }
            else if(DbgGetModuleAt(addr, module) && !QString(label).startsWith("JMP.&"))
                addrText = QString(module) + "." + QString("%1").arg(addr, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
            else
                addrText = QString("%1").arg(addr, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
            ui->labelError->setText(QString("<font color='#00DD00'><b>Correct expression! -> </b></font>" + addrText));
            ui->buttonOk->setEnabled(true);
            expressionText = expression;
        }
    }
}

void GotoDialog::on_editExpression_textChanged(const QString & arg1)
{
    Q_UNUSED(arg1);
    ui->buttonOk->setEnabled(false);
}

bool GotoDialog::IsValidMemoryRange(uint_t addr)
{
    return ((!validRangeStart && !validRangeEnd) || (addr >= validRangeStart && addr < validRangeEnd));
}

void GotoDialog::on_buttonOk_clicked()
{
    QString expression = ui->editExpression->text();
    ui->editExpression->addLineToHistory(expression);
    ui->editExpression->setText("");
    expressionText = expression;
}

void GotoDialog::finishedSlot(int result)
{
    if(result == QDialog::Rejected)
        ui->editExpression->setText("");
    ui->editExpression->setFocus();
}
