#include "SettingsDialog.h"
#include "ui_SettingsDialog.h"
#include <QMessageBox>
#include "Configuration.h"
#include "Bridge.h"
#include "ExceptionRangeDialog.h"

SettingsDialog::SettingsDialog(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    //set window flags
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
    setWindowFlags(Qt::Dialog | Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::MSWindowsFixedSizeDialogHint);
#endif
    setFixedSize(this->size()); //fixed size
    setModal(true);
    LoadSettings(); //load settings from file
    connect(Bridge::getBridge(), SIGNAL(setLastException(uint)), this, SLOT(setLastException(uint)));
    lastException = 0;
}

SettingsDialog::~SettingsDialog()
{
    disconnect(Bridge::getBridge(), SIGNAL(setLastException(uint)), this, SLOT(setLastException(uint)));
    delete ui;
}

void SettingsDialog::GetSettingBool(const char* section, const char* name, bool* set)
{
    duint currentSetting;
    if(!set || !BridgeSettingGetUint(section, name, &currentSetting))
        return;
    if(currentSetting)
        *set = true;
    else
        *set = false;
}

Qt::CheckState SettingsDialog::bool2check(bool checked)
{
    if(checked)
        return Qt::Checked;
    return Qt::Unchecked;
}

void SettingsDialog::LoadSettings()
{
    //Defaults
    memset(&settings, 0, sizeof(SettingsStruct));
    settings.eventSystemBreakpoint = true;
    settings.eventTlsCallbacks = true;
    settings.eventEntryBreakpoint = true;
    settings.eventAttachBreakpoint = true;
    settings.engineCalcType = calc_unsigned;
    settings.engineBreakpointType = break_int3short;
    settings.engineUndecorateSymbolNames = true;
    settings.exceptionRanges = &realExceptionRanges;
    settings.disasmArgumentSpaces = false;
    settings.disasmMemorySpaces = false;
    settings.disasmUppercase = false;
    settings.disasmOnlyCipAutoComments = false;

    //Events tab
    GetSettingBool("Events", "SystemBreakpoint", &settings.eventSystemBreakpoint);
    GetSettingBool("Events", "TlsCallbacks", &settings.eventTlsCallbacks);
    GetSettingBool("Events", "EntryBreakpoint", &settings.eventEntryBreakpoint);
    GetSettingBool("Events", "DllEntry", &settings.eventDllEntry);
    GetSettingBool("Events", "ThreadEntry", &settings.eventThreadEntry);
    GetSettingBool("Events", "AttachBreakpoint", &settings.eventAttachBreakpoint);
    GetSettingBool("Events", "DllLoad", &settings.eventDllLoad);
    GetSettingBool("Events", "DllUnload", &settings.eventDllUnload);
    GetSettingBool("Events", "ThreadStart", &settings.eventThreadStart);
    GetSettingBool("Events", "ThreadEnd", &settings.eventThreadEnd);
    GetSettingBool("Events", "DebugStrings", &settings.eventDebugStrings);
    ui->chkSystemBreakpoint->setCheckState(bool2check(settings.eventSystemBreakpoint));
    ui->chkTlsCallbacks->setCheckState(bool2check(settings.eventTlsCallbacks));
    ui->chkEntryBreakpoint->setCheckState(bool2check(settings.eventEntryBreakpoint));
    ui->chkDllEntry->setCheckState(bool2check(settings.eventDllEntry));
    ui->chkThreadEntry->setCheckState(bool2check(settings.eventThreadEntry));
    ui->chkAttachBreakpoint->setCheckState(bool2check(settings.eventAttachBreakpoint));
    ui->chkDllLoad->setCheckState(bool2check(settings.eventDllLoad));
    ui->chkDllUnload->setCheckState(bool2check(settings.eventDllUnload));
    ui->chkThreadStart->setCheckState(bool2check(settings.eventThreadStart));
    ui->chkThreadEnd->setCheckState(bool2check(settings.eventThreadEnd));
    ui->chkDebugStrings->setCheckState(bool2check(settings.eventDebugStrings));

    //Engine tab
    duint cur;
    if(BridgeSettingGetUint("Engine", "CalculationType", &cur))
    {
        switch(cur)
        {
        case calc_signed:
        case calc_unsigned:
            settings.engineCalcType = (CalcType)cur;
            break;
        }
    }
    if(BridgeSettingGetUint("Engine", "BreakpointType", &cur))
    {
        switch(cur)
        {
        case break_int3short:
        case break_int3long:
        case break_ud2:
            settings.engineBreakpointType = (BreakpointType)cur;
            break;
        }
    }
    GetSettingBool("Engine", "UndecorateSymbolNames", &settings.engineUndecorateSymbolNames);
    GetSettingBool("Engine", "EnableDebugPrivilege", &settings.engineEnableDebugPrivilege);
    switch(settings.engineCalcType)
    {
    case calc_signed:
        ui->radioSigned->setChecked(true);
        break;
    case calc_unsigned:
        ui->radioUnsigned->setChecked(true);
        break;
    }
    switch(settings.engineBreakpointType)
    {
    case break_int3short:
        ui->radioInt3Short->setChecked(true);
        break;
    case break_int3long:
        ui->radioInt3Long->setChecked(true);
        break;
    case break_ud2:
        ui->radioUd2->setChecked(true);
        break;
    }
    ui->chkUndecorateSymbolNames->setChecked(settings.engineUndecorateSymbolNames);
    ui->chkEnableDebugPrivilege->setChecked(settings.engineEnableDebugPrivilege);

    //Exceptions tab
    char exceptionRange[MAX_SETTING_SIZE] = "";
    if(BridgeSettingGet("Exceptions", "IgnoreRange", exceptionRange))
    {
        QStringList ranges = QString(exceptionRange).split(QString(","), QString::SkipEmptyParts);
        for(int i = 0; i < ranges.size(); i++)
        {
            unsigned long start;
            unsigned long end;
            if(sscanf_s(ranges.at(i).toUtf8().constData(), "%08X-%08X", &start, &end) == 2 && start <= end)
            {
                RangeStruct newRange;
                newRange.start = start;
                newRange.end = end;
                AddRangeToList(newRange);
            }
        }
    }

    //Disasm tab
    GetSettingBool("Disassembler", "ArgumentSpaces", &settings.disasmArgumentSpaces);
    GetSettingBool("Disassembler", "MemorySpaces", &settings.disasmMemorySpaces);
    GetSettingBool("Disassembler", "Uppercase", &settings.disasmUppercase);
    GetSettingBool("Disassembler", "OnlyCipAutoComments", &settings.disasmOnlyCipAutoComments);
    ui->chkArgumentSpaces->setChecked(settings.disasmArgumentSpaces);
    ui->chkMemorySpaces->setChecked(settings.disasmMemorySpaces);
    ui->chkUppercase->setChecked(settings.disasmUppercase);
    ui->chkOnlyCipAutoComments->setChecked(settings.disasmOnlyCipAutoComments);

    //Misc tab
    if(DbgFunctions()->GetJit)
    {
        char jit_entry[MAX_SETTING_SIZE] = "";
        char jit_def_entry[MAX_SETTING_SIZE] = "";
        bool isx64 = true;
#ifndef _WIN64
        isx64 = false;
#endif
        bool jit_auto_on;
        bool get_jit_works;
        get_jit_works = DbgFunctions()->GetJit(jit_entry, isx64);
        DbgFunctions()->GetDefJit(jit_def_entry);

        if(get_jit_works)
        {
            if(_strcmpi(jit_entry, jit_def_entry) == 0)
                settings.miscSetJIT = true;
        }
        else
            settings.miscSetJIT = false;
        ui->editJIT->setText(jit_entry);
        ui->editJIT->setCursorPosition(0);

        ui->chkSetJIT->setCheckState(bool2check(settings.miscSetJIT));

        bool get_jit_auto_works = DbgFunctions()->GetJitAuto(&jit_auto_on);
        if(!get_jit_auto_works || !jit_auto_on)
            settings.miscSetJITAuto = true;
        else
            settings.miscSetJITAuto = false;

        ui->chkConfirmBeforeAtt->setCheckState(bool2check(settings.miscSetJITAuto));

        if(!DbgFunctions()->IsProcessElevated())
        {
            ui->chkSetJIT->setDisabled(true);
            ui->chkConfirmBeforeAtt->setDisabled(true);
            ui->lbladminwarning->setText(QString("Warning: Run the debugger as Admin to enable JIT."));
        }
    }

    bJitOld = settings.miscSetJIT;
    bJitAutoOld = settings.miscSetJITAuto;
}

void SettingsDialog::SaveSettings()
{
    //Events tab
    BridgeSettingSetUint("Events", "SystemBreakpoint", settings.eventSystemBreakpoint);
    BridgeSettingSetUint("Events", "TlsCallbacks", settings.eventTlsCallbacks);
    BridgeSettingSetUint("Events", "EntryBreakpoint", settings.eventEntryBreakpoint);
    BridgeSettingSetUint("Events", "DllEntry", settings.eventDllEntry);
    BridgeSettingSetUint("Events", "ThreadEntry", settings.eventThreadEntry);
    BridgeSettingSetUint("Events", "AttachBreakpoint", settings.eventAttachBreakpoint);
    BridgeSettingSetUint("Events", "DllLoad", settings.eventDllLoad);
    BridgeSettingSetUint("Events", "DllUnload", settings.eventDllUnload);
    BridgeSettingSetUint("Events", "ThreadStart", settings.eventThreadStart);
    BridgeSettingSetUint("Events", "ThreadEnd", settings.eventThreadEnd);
    BridgeSettingSetUint("Events", "DebugStrings", settings.eventDebugStrings);

    //Engine tab
    BridgeSettingSetUint("Engine", "CalculationType", settings.engineCalcType);
    BridgeSettingSetUint("Engine", "BreakpointType", settings.engineBreakpointType);
    BridgeSettingSetUint("Engine", "UndecorateSymbolNames", settings.engineUndecorateSymbolNames);
    BridgeSettingSetUint("Engine", "EnableDebugPrivilege", settings.engineEnableDebugPrivilege);

    //Exceptions tab
    QString exceptionRange = "";
    for(int i = 0; i < settings.exceptionRanges->size(); i++)
        exceptionRange.append(QString().sprintf("%.8X-%.8X", settings.exceptionRanges->at(i).start, settings.exceptionRanges->at(i).end) + QString(","));
    exceptionRange.chop(1); //remove last comma
    if(exceptionRange.size())
        BridgeSettingSet("Exceptions", "IgnoreRange", exceptionRange.toUtf8().constData());
    else
        BridgeSettingSet("Exceptions", "IgnoreRange", "");

    //Disasm tab
    BridgeSettingSetUint("Disassembler", "ArgumentSpaces", settings.disasmArgumentSpaces);
    BridgeSettingSetUint("Disassembler", "MemorySpaces", settings.disasmMemorySpaces);
    BridgeSettingSetUint("Disassembler", "Uppercase", settings.disasmUppercase);
    BridgeSettingSetUint("Disassembler", "OnlyCipAutoComments", settings.disasmOnlyCipAutoComments);

    //Misc tab
    if(DbgFunctions()->GetJit)
    {
        if(bJitOld != settings.miscSetJIT)
        {
            if(settings.miscSetJIT)
                DbgCmdExecDirect("setjit oldsave");
            else
                DbgCmdExecDirect("setjit restore");
        }

        if(bJitAutoOld != settings.miscSetJITAuto)
        {
            if(!settings.miscSetJITAuto)
                DbgCmdExecDirect("setjitauto on");
            else
                DbgCmdExecDirect("setjitauto off");
        }
    }

    Config()->load();
    DbgSettingsUpdated();
    GuiUpdateAllViews();
}

void SettingsDialog::AddRangeToList(RangeStruct range)
{
    //check range
    unsigned long start = range.start;
    unsigned long end = range.end;

    for(int i = settings.exceptionRanges->size() - 1; i > -1; i--)
    {
        unsigned long curStart = settings.exceptionRanges->at(i).start;
        unsigned long curEnd = settings.exceptionRanges->at(i).end;
        if(curStart <= end && curEnd >= start) //ranges overlap
        {
            if(curStart < start) //extend range to the left
                start = curStart;
            if(curEnd > end) //extend range to the right
                end = curEnd;
            settings.exceptionRanges->erase(settings.exceptionRanges->begin() + i); //remove old range
        }
    }
    range.start = start;
    range.end = end;
    settings.exceptionRanges->push_back(range);
    qSort(settings.exceptionRanges->begin(), settings.exceptionRanges->end(), RangeStructLess());
    ui->listExceptions->clear();
    for(int i = 0; i < settings.exceptionRanges->size(); i++)
        ui->listExceptions->addItem(QString().sprintf("%.8X-%.8X", settings.exceptionRanges->at(i).start, settings.exceptionRanges->at(i).end));
}

void SettingsDialog::setLastException(unsigned int exceptionCode)
{
    lastException = exceptionCode;
}

void SettingsDialog::on_btnSave_clicked()
{
    SaveSettings();
    GuiAddStatusBarMessage("Settings saved!\n");
}

void SettingsDialog::on_chkSystemBreakpoint_stateChanged(int arg1)
{
    if(arg1 == Qt::Unchecked)
        settings.eventSystemBreakpoint = false;
    else
        settings.eventSystemBreakpoint = true;
}

void SettingsDialog::on_chkTlsCallbacks_stateChanged(int arg1)
{
    if(arg1 == Qt::Unchecked)
        settings.eventTlsCallbacks = false;
    else
        settings.eventTlsCallbacks = true;
}

void SettingsDialog::on_chkEntryBreakpoint_stateChanged(int arg1)
{
    if(arg1 == Qt::Unchecked)
        settings.eventEntryBreakpoint = false;
    else
        settings.eventEntryBreakpoint = true;
}

void SettingsDialog::on_chkDllEntry_stateChanged(int arg1)
{
    if(arg1 == Qt::Unchecked)
        settings.eventDllEntry = false;
    else
        settings.eventDllEntry = true;
}

void SettingsDialog::on_chkThreadEntry_stateChanged(int arg1)
{
    if(arg1 == Qt::Unchecked)
        settings.eventThreadEntry = false;
    else
        settings.eventThreadEntry = true;
}

void SettingsDialog::on_chkAttachBreakpoint_stateChanged(int arg1)
{
    if(arg1 == Qt::Unchecked)
        settings.eventAttachBreakpoint = false;
    else
        settings.eventAttachBreakpoint = true;
}

void SettingsDialog::on_chkConfirmBeforeAtt_stateChanged(int arg1)
{
    if(arg1 == Qt::Unchecked)
        settings.miscSetJITAuto = false;
    else
        settings.miscSetJITAuto = true;
}

void SettingsDialog::on_chkSetJIT_stateChanged(int arg1)
{
    if(arg1 == Qt::Unchecked)
    {
        if(DbgFunctions()->GetJit)
        {
            char jit_def_entry[MAX_SETTING_SIZE] = "";
            QString qsjit_def_entry;

            DbgFunctions()->GetDefJit(jit_def_entry);

            qsjit_def_entry = jit_def_entry;

            // if there are not an OLD JIT Stored GetJit(NULL,) returns false.
            if((DbgFunctions()->GetJit(NULL, true) == false) && (ui->editJIT->text() == qsjit_def_entry))
            {
                /*
                 * Only do this when the user wants uncheck the JIT and there are not an OLD JIT Stored
                 * and the JIT in Windows registry its this debugger.
                 * Scenario 1: the JIT in Windows registry its this debugger, if the database of the
                 * debugger was removed and the user wants uncheck the JIT: he cant (this block its executed then)
                 * -
                 * Scenario 2: the JIT in Windows registry its NOT this debugger, if the database of the debugger
                 * was removed and the user in MISC tab wants check and uncheck the JIT checkbox: he can (this block its NOT executed then).
                */
                QMessageBox msg(QMessageBox::Warning, "ERROR NOT FOUND OLD JIT", "NOT FOUND OLD JIT ENTRY STORED, USE SETJIT COMMAND");
                msg.setWindowIcon(QIcon(":/icons/images/compile-warning.png"));
                msg.setParent(this, Qt::Dialog);
                msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
                msg.exec();

                settings.miscSetJIT = true;
            }
            else
                settings.miscSetJIT = false;

            ui->chkSetJIT->setCheckState(bool2check(settings.miscSetJIT));
        }
        settings.miscSetJIT = false;
    }
    else
        settings.miscSetJIT = true;
}

void SettingsDialog::on_chkDllLoad_stateChanged(int arg1)
{
    if(arg1 == Qt::Unchecked)
        settings.eventDllLoad = false;
    else
        settings.eventDllLoad = true;
}

void SettingsDialog::on_chkDllUnload_stateChanged(int arg1)
{
    if(arg1 == Qt::Unchecked)
        settings.eventDllUnload = false;
    else
        settings.eventDllUnload = true;
}

void SettingsDialog::on_chkThreadStart_stateChanged(int arg1)
{
    if(arg1 == Qt::Unchecked)
        settings.eventThreadStart = false;
    else
        settings.eventThreadStart = true;
}

void SettingsDialog::on_chkThreadEnd_stateChanged(int arg1)
{
    if(arg1 == Qt::Unchecked)
        settings.eventThreadEnd = false;
    else
        settings.eventThreadEnd = true;
}

void SettingsDialog::on_chkDebugStrings_stateChanged(int arg1)
{
    if(arg1 == Qt::Unchecked)
        settings.eventDebugStrings = false;
    else
        settings.eventDebugStrings = true;
}

void SettingsDialog::on_radioUnsigned_clicked()
{
    settings.engineCalcType = calc_unsigned;
}

void SettingsDialog::on_radioSigned_clicked()
{
    settings.engineCalcType = calc_signed;
}

void SettingsDialog::on_radioInt3Short_clicked()
{
    settings.engineBreakpointType = break_int3short;
}

void SettingsDialog::on_radioInt3Long_clicked()
{
    settings.engineBreakpointType = break_int3long;
}

void SettingsDialog::on_radioUd2_clicked()
{
    settings.engineBreakpointType = break_ud2;
}

void SettingsDialog::on_chkUndecorateSymbolNames_stateChanged(int arg1)
{
    if(arg1 == Qt::Unchecked)
        settings.engineUndecorateSymbolNames = false;
    else
        settings.engineUndecorateSymbolNames = true;
}

void SettingsDialog::on_chkEnableDebugPrivilege_stateChanged(int arg1)
{
    if(arg1 == Qt::Unchecked)
        settings.engineEnableDebugPrivilege = false;
    else
        settings.engineEnableDebugPrivilege = true;
}

void SettingsDialog::on_btnAddRange_clicked()
{
    ExceptionRangeDialog exceptionRange(this);
    if(exceptionRange.exec() != QDialog::Accepted)
        return;
    RangeStruct range;
    range.start = exceptionRange.rangeStart;
    range.end = exceptionRange.rangeEnd;
    AddRangeToList(range);
}

void SettingsDialog::on_btnDeleteRange_clicked()
{
    QModelIndexList indexes = ui->listExceptions->selectionModel()->selectedIndexes();
    if(!indexes.size()) //no selection
        return;
    settings.exceptionRanges->erase(settings.exceptionRanges->begin() + indexes.at(0).row());
    ui->listExceptions->clear();
    for(int i = 0; i < settings.exceptionRanges->size(); i++)
        ui->listExceptions->addItem(QString().sprintf("%.8X-%.8X", settings.exceptionRanges->at(i).start, settings.exceptionRanges->at(i).end));
}

void SettingsDialog::on_btnAddLast_clicked()
{
    QMessageBox msg(QMessageBox::Question, "Question", QString().sprintf("Are you sure you want to add %.8X?", lastException));
    msg.setWindowIcon(QIcon(":/icons/images/question.png"));
    msg.setParent(this, Qt::Dialog);
    msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
    msg.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
    msg.setDefaultButton(QMessageBox::Yes);
    if(msg.exec() != QMessageBox::Yes)
        return;
    RangeStruct range;
    range.start = lastException;
    range.end = lastException;
    AddRangeToList(range);
}

void SettingsDialog::on_chkArgumentSpaces_stateChanged(int arg1)
{
    if(arg1 == Qt::Unchecked)
        settings.disasmArgumentSpaces = false;
    else
        settings.disasmArgumentSpaces = true;
}

void SettingsDialog::on_chkMemorySpaces_stateChanged(int arg1)
{
    if(arg1 == Qt::Unchecked)
        settings.disasmMemorySpaces = false;
    else
        settings.disasmMemorySpaces = true;
}

void SettingsDialog::on_chkUppercase_stateChanged(int arg1)
{
    if(arg1 == Qt::Unchecked)
        settings.disasmUppercase = false;
    else
        settings.disasmUppercase = true;
}

void SettingsDialog::on_chkOnlyCipAutoComments_stateChanged(int arg1)
{
    if(arg1 == Qt::Unchecked)
        settings.disasmOnlyCipAutoComments = false;
    else
        settings.disasmOnlyCipAutoComments = true;
}
