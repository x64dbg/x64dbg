#include "CPUArgumentWidget.h"
#include "ui_CPUArgumentWidget.h"
#include "Configuration.h"
#include <QDebug>

CPUArgumentWidget::CPUArgumentWidget(QWidget* parent) :
    QWidget(parent),
    ui(new Ui::CPUArgumentWidget),
    mTable(nullptr),
    mCurrentCallingConvention(-1),
    mStackOffset(0),
    mAllowUpdate(true)
{
    ui->setupUi(this);
    mTable = ui->table;
    setupTable();
    loadConfig();
    refreshData();
}

CPUArgumentWidget::~CPUArgumentWidget()
{
    delete ui;
}

void CPUArgumentWidget::disassembledAtSlot(dsint, dsint cip, bool, dsint)
{
    if(mCurrentCallingConvention == -1) //no calling conventions
    {
        mTable->setRowCount(0);
        mTable->reloadData();
        return;
    }

    BASIC_INSTRUCTION_INFO disasm;
    DbgDisasmFastAt(cip, &disasm);

    const auto & cur = mCallingConventions[mCurrentCallingConvention];
    mStackOffset = disasm.call ? 0 : cur.getCallOffset();
    if(ui->checkBoxLock->checkState() == Qt::PartiallyChecked) //Calls
    {
        mAllowUpdate = disasm.call;
        ui->spinArgCount->setEnabled(disasm.call);
        ui->comboCallingConvention->setEnabled(disasm.call);
    }
}

static QString stringFormatInline(const QString & format)
{
    if(!DbgFunctions()->StringFormatInline)
        return "";
    char result[MAX_SETTING_SIZE] = "";
    if(DbgFunctions()->StringFormatInline(format.toUtf8().constData(), MAX_SETTING_SIZE, result))
        return result;
    return "[Formatting Error]";

}

void CPUArgumentWidget::refreshData()
{
    if(!mAllowUpdate) //view is locked
        return;

    if(mCurrentCallingConvention == -1) //no calling conventions
    {
        mTable->setRowCount(0);
        mTable->reloadData();
        return;
    }

    const auto & cur = mCallingConventions[mCurrentCallingConvention];
    int argCountStruct = int(cur.arguments.size());
    int argCount = std::min(argCountStruct, ui->spinArgCount->value());
    int stackCount = std::max(0, ui->spinArgCount->value() - argCountStruct);
    mTable->setRowCount(argCount + stackCount);

    for(int i = 0; i < argCount; i++)
    {
        const auto & curArg = cur.arguments[i];
        auto data = stringFormatInline(curArg.getFormat());
        auto text = defaultArgFieldFormat(defaultArgName(curArg.name, i + 1), data);
        mTable->setCellContent(i, 0, text);
    }

    auto stackLocation = cur.getStackLocation();
    for(int i = 0; i < stackCount; i++)
    {
        duint argOffset = mStackOffset + i * sizeof(duint);
        QString expr = argOffset ? QString("%1+%2").arg(stackLocation).arg(ToHexString(argOffset)) : stackLocation;

        QString format = defaultArgFormat("", QString("[%1]").arg(expr));
        auto data = stringFormatInline(format);
        auto text = defaultArgFieldFormat(defaultArgName("", argCount + i + 1), data);
        mTable->setCellContent(argCount + i, 0, text);
    }

    mTable->reloadData();
}

void CPUArgumentWidget::loadConfig()
{
#ifdef _WIN64
    CallingConvention x64("Default (x64 fastcall)", 1);
    x64.addArgument(Argument("", "", "rcx"));
    x64.addArgument(Argument("", "", "rdx"));
    x64.addArgument(Argument("", "", "r8"));
    x64.addArgument(Argument("", "", "r9"));
    mCallingConventions.push_back(x64);
#else
    CallingConvention x32("Default (stdcall)", 5);
    mCallingConventions.push_back(x32);

    CallingConvention x32ebp("Default (stdcall, EBP stack)", 5, "ebp");
    mCallingConventions.push_back(x32ebp);

    CallingConvention thiscall("thiscall", 4);
    thiscall.addArgument(Argument("this", "ecx", ""));
    mCallingConventions.push_back(thiscall);

    CallingConvention delphi("Delphi (Borland fastcall)", 2);
    delphi.addArgument(Argument("", "eax", ""));
    delphi.addArgument(Argument("", "edx", ""));
    delphi.addArgument(Argument("", "ecx", ""));
    mCallingConventions.push_back(delphi);
#endif //_WIN64

    for(auto & cur : mCallingConventions)
        ui->comboCallingConvention->addItem(cur.name);
}

void CPUArgumentWidget::setupTable()
{
    mTable->verticalScrollBar()->setStyleSheet(ConfigVScrollBarStyle());
    mTable->enableMultiSelection(false);
    mTable->setShowHeader(false);
    mTable->addColumnAt(0, "", false);
    mTable->reloadData();
}

void CPUArgumentWidget::on_comboCallingConvention_currentIndexChanged(int index)
{
    mCurrentCallingConvention = index;
    const auto & cur = mCallingConventions[index];
    ui->spinArgCount->setValue(int(cur.arguments.size()) + cur.stackArgCount); //set the default argument count
    refreshData();
}

void CPUArgumentWidget::on_spinArgCount_valueChanged(int)
{
    mTable->setTableOffset(0); //reset the view to the first argument (fixes an ugly issue after refreshing)
    refreshData();
}

void CPUArgumentWidget::on_checkBoxLock_stateChanged(int)
{
    switch(ui->checkBoxLock->checkState())
    {
    case Qt::Checked:
        refreshData(); //first refresh then lock
        ui->checkBoxLock->setText("Locked");
        ui->spinArgCount->setEnabled(false);
        ui->comboCallingConvention->setEnabled(false);
        mAllowUpdate = false;
        break;
    case Qt::PartiallyChecked:
        refreshData(); //first refresh then lock
        ui->checkBoxLock->setText("Calls");
        ui->spinArgCount->setEnabled(false);
        ui->comboCallingConvention->setEnabled(false);
        mAllowUpdate = false;
        break;
    case Qt::Unchecked:
        ui->checkBoxLock->setText("Unlocked");
        ui->spinArgCount->setEnabled(true);
        ui->comboCallingConvention->setEnabled(true);
        mAllowUpdate = true;
        refreshData(); //first lock then refresh
        break;
    default:
        break;
    }
}
