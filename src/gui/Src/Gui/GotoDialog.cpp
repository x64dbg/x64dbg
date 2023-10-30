#include "GotoDialog.h"
#include "ValidateExpressionThread.h"
#include "ui_GotoDialog.h"
#include "StringUtil.h"
#include "Configuration.h"
#include "QCompleter"
#include "SymbolAutoCompleteModel.h"

GotoDialog::GotoDialog(QWidget* parent, bool allowInvalidExpression, bool allowInvalidAddress, bool allowNotDebugging)
    : QDialog(parent),
      ui(new Ui::GotoDialog),
      allowInvalidExpression(allowInvalidExpression),
      allowInvalidAddress(allowInvalidAddress || allowInvalidExpression),
      allowNotDebugging(allowNotDebugging)
{
    //setup UI first
    ui->setupUi(this);
    setModal(true);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint | Qt::MSWindowsFixedSizeDialogHint);

    //initialize stuff
    if(!allowNotDebugging && !DbgIsDebugging()) //not debugging
        ui->labelError->setText(tr("<font color='red'><b>Not debugging...</b></font>"));
    else
        ui->labelError->setText(tr("<font color='red'><b>Invalid expression...</b></font>"));
    setOkEnabled(false);
    ui->editExpression->setFocus();
    completer = new QCompleter(this);
    completer->setModel(new SymbolAutoCompleteModel([this]
    {
        return mCompletionText;
    }, completer));
    completer->setCaseSensitivity(Config()->getBool("Gui", "CaseSensitiveAutoComplete") ? Qt::CaseSensitive : Qt::CaseInsensitive);
    if(!Config()->getBool("Gui", "DisableAutoComplete"))
        ui->editExpression->setCompleter(completer);
    validRangeStart = 0;
    validRangeEnd = ~0;
    fileOffset = false;
    mValidateThread = new ValidateExpressionThread(this);
    mValidateThread->setOnExpressionChangedCallback(std::bind(&GotoDialog::validateExpression, this, std::placeholders::_1));

    connect(mValidateThread, SIGNAL(expressionChanged(bool, bool, dsint)), this, SLOT(expressionChanged(bool, bool, dsint)));
    connect(ui->editExpression, SIGNAL(textChanged(QString)), mValidateThread, SLOT(textChanged(QString)));
    connect(ui->editExpression, SIGNAL(textEdited(QString)), this, SLOT(textEditedSlot(QString)));
    connect(ui->labelError, SIGNAL(linkActivated(QString)), this, SLOT(linkActivated(QString)));
    connect(this, SIGNAL(finished(int)), this, SLOT(finishedSlot(int)));
    connect(Config(), SIGNAL(disableAutoCompleteUpdated()), this, SLOT(disableAutoCompleteUpdated()));

    Config()->loadWindowGeometry(this);
}

GotoDialog::~GotoDialog()
{
    mValidateThread->stop();
    mValidateThread->wait();
    Config()->saveWindowGeometry(this);
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
    mValidateThread->stop();
    mValidateThread->wait();
}

void GotoDialog::validateExpression(QString expression)
{
    duint value;
    bool validExpression = DbgFunctions()->ValFromString(expression.toUtf8().constData(), &value);
    unsigned char ch;
    bool validPointer = validExpression && DbgMemIsValidReadPtr(value) && DbgMemRead(value, &ch, sizeof(ch));
    this->mValidateThread->emitExpressionChanged(validExpression, validPointer, value);
}

void GotoDialog::setInitialExpression(const QString & expression)
{
    ui->editExpression->setText(expression);
    emit ui->editExpression->textEdited(expression);
}

static QString breakWithLines(const unsigned int numberCharsPerLine, const QString & txt, const bool condBreakBefore)
{
    const QString BRStr = QString("<br>");
    const unsigned int breakCount = txt.size() / numberCharsPerLine;
    QString result = txt;
    
    for(unsigned int i = 1 ; i <= breakCount; i++)
    {
        unsigned int charactersToSkip = (numberCharsPerLine + BRStr.size()) * i - BRStr.size();
        result = result.left(charactersToSkip) + BRStr + result.right(result.size() - charactersToSkip);
    }

    if(condBreakBefore && breakCount >= 1)
        result = BRStr + result;

    return result;
}

void GotoDialog::expressionChanged(bool validExpression, bool validPointer, dsint value)
{
    QString expression = ui->editExpression->text();
    if(!expression.length())
    {
        QString tips[] = {"RVA", tr("File offset"), "PEB", "TEB"};
        const char* expressions[] = {":$%", ":#%", "peb()", "teb()"};
        QString labelText(tr("Shortcuts: "));
        for(size_t i = 0; i < 4; i++)
        {
            labelText += "<a href=\"";
            labelText += QString(expressions[i]).replace('%', "%" + tips[i] + "%") + "\">" + QString(expressions[i]).replace('%', tips[i]) + "</a>";
            if(i < 3)
                labelText += '\t';
        }
        ui->labelError->setText(labelText);
        setOkEnabled(false);
        expressionText.clear();
    }
    if(expressionText == expression)
        return;
    if(!allowNotDebugging && !DbgIsDebugging()) //not debugging
    {
        ui->labelError->setText(tr("<font color='red'><b>Not debugging...</b></font>"));
        setOkEnabled(false);
        expressionText.clear();
    }
    else if(!validExpression) //invalid expression
    {
        ui->labelError->setText(tr("<font color='red'><b>Invalid expression...</b></font>"));
        setOkEnabled(false);
        expressionText.clear();
    }
    else if(fileOffset)
    {
        duint offset = value;
        duint va = DbgFunctions()->FileOffsetToVa(modName.toUtf8().constData(), offset);
        QString addrText = QString(" %1").arg(ToPtrString(va));
        if(va || allowInvalidAddress)
        {
            ui->labelError->setText(tr("<font color='#00DD00'><b>Correct expression! -&gt; </b></font>") + addrText);
            setOkEnabled(true);
            expressionText = expression;
        }
        else
        {
            ui->labelError->setText(tr("<font color='red'><b>Invalid file offset...</b></font>") + addrText);
            setOkEnabled(false);
            expressionText.clear();
        }
    }
    else
    {
        duint addr = value;
        QString addrText = QString(" %1").arg(ToPtrString(addr));
        if(!validPointer && !allowInvalidAddress)
        {
            ui->labelError->setText(tr("<font color='red'><b>Invalid memory address...</b></font>") + addrText);
            setOkEnabled(false);
            expressionText.clear();
        }
        else if(!IsValidMemoryRange(addr) && !allowInvalidAddress)
        {
            ui->labelError->setText(tr("<font color='red'><b>Memory out of range...</b></font>") + addrText);
            setOkEnabled(false);
            expressionText.clear();
        }
        else
        {
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
                addrText = QString(module) + "." + ToPtrString(addr);
            else
                addrText = ToPtrString(addr);

            addrText = breakWithLines(ui->editExpression->size().width() / 11, addrText, true); // 11 is a good value for WWWWWWWs
            ui->labelError->setText(tr("<font color='#00DD00'><b>Correct expression! -&gt; </b></font>") + addrText);
            setOkEnabled(true);
            expressionText = expression;
        }
    }
}

bool GotoDialog::IsValidMemoryRange(duint addr)
{
    return addr >= validRangeStart && addr < validRangeEnd;
}

void GotoDialog::setOkEnabled(bool enabled)
{
    ui->buttonOk->setEnabled(enabled || allowInvalidExpression);
}

void GotoDialog::on_buttonOk_clicked()
{
    QString expression = ui->editExpression->text();
    ui->editExpression->addLineToHistory(expression);
    ui->editExpression->setText("");
    expressionChanged(false, false, 0);
    expressionText = expression;
}

void GotoDialog::finishedSlot(int result)
{
    if(result == QDialog::Rejected)
        ui->editExpression->setText("");
    ui->editExpression->setFocus();
}

void GotoDialog::textEditedSlot(QString text)
{
    mCompletionText = text;
}

void GotoDialog::linkActivated(const QString & link)
{
    if(link.contains('%'))
    {
        int x = link.indexOf('%');
        int y = link.lastIndexOf('%');
        ui->editExpression->setText(QString(link).replace('%', ""));
        ui->editExpression->setSelection(x, y - 1);
    }
    else
        ui->editExpression->setText(link);
}

void GotoDialog::disableAutoCompleteUpdated()
{
    if(Config()->getBool("Gui", "DisableAutoComplete"))
        ui->editExpression->setCompleter(nullptr);
    else
        ui->editExpression->setCompleter(completer);
}
