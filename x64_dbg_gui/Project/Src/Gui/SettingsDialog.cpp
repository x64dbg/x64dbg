#include "SettingsDialog.h"
#include "ui_SettingsDialog.h"

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    //set window flags
    setModal(true);
    setWindowFlags(Qt::Dialog | Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::MSWindowsFixedSizeDialogHint);
    setFixedSize(this->size()); //fixed size
    LoadSettings(); //load settings from file
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::GetSettingBool(const char* section, const char* name, bool* set)
{
    duint currentSetting;
    if(!set || !BridgeSettingGetUint(section, name, &currentSetting))
        return;
    if(currentSetting)
        *set=true;
    else
        *set=false;
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
    settings.eventSystemBreakpoint=true;
    settings.eventTlsCallbacks=true;
    settings.eventEntryBreakpoint=true;
    settings.engineCalcType=calc_unsigned;
    settings.engineBreakpointType=break_int3short;

    //Events tab
    GetSettingBool("Events", "SystemBreakpoint", &settings.eventSystemBreakpoint);
    GetSettingBool("Events", "TlsCallbacks", &settings.eventTlsCallbacks);
    GetSettingBool("Events", "EntryBreakpoint", &settings.eventEntryBreakpoint);
    GetSettingBool("Events", "DllEntry", &settings.eventDllEntry);
    GetSettingBool("Events", "ThreadEntry", &settings.eventThreadEntry);
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
            settings.engineCalcType=(CalcType)cur;
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
            settings.engineBreakpointType=(BreakpointType)cur;
            break;
        }
    }
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
}

void SettingsDialog::SaveSettings()
{
    //Events tab
    BridgeSettingSetUint("Events", "SystemBreakpoint", settings.eventSystemBreakpoint);
    BridgeSettingSetUint("Events", "TlsCallbacks", settings.eventTlsCallbacks);
    BridgeSettingSetUint("Events", "EntryBreakpoint", settings.eventEntryBreakpoint);
    BridgeSettingSetUint("Events", "DllEntry", settings.eventDllEntry);
    BridgeSettingSetUint("Events", "ThreadEntry", settings.eventThreadEntry);
    BridgeSettingSetUint("Events", "DllLoad", settings.eventDllLoad);
    BridgeSettingSetUint("Events", "DllUnload", settings.eventDllUnload);
    BridgeSettingSetUint("Events", "ThreadStart", settings.eventThreadStart);
    BridgeSettingSetUint("Events", "ThreadEnd", settings.eventThreadEnd);
    BridgeSettingSetUint("Events", "DebugStrings", settings.eventDebugStrings);

    //Engine tab
    BridgeSettingSetUint("Engine", "CalculationType", settings.engineCalcType);
    BridgeSettingSetUint("Engine", "BreakpointType", settings.engineBreakpointType);
}

void SettingsDialog::on_chkSystemBreakpoint_stateChanged(int arg1)
{
    if(arg1==Qt::Unchecked)
        settings.eventSystemBreakpoint=false;
    else
        settings.eventSystemBreakpoint=true;
}

void SettingsDialog::on_chkTlsCallbacks_stateChanged(int arg1)
{
    if(arg1==Qt::Unchecked)
        settings.eventTlsCallbacks=false;
    else
        settings.eventTlsCallbacks=true;
}

void SettingsDialog::on_chkEntryBreakpoint_stateChanged(int arg1)
{
    if(arg1==Qt::Unchecked)
        settings.eventEntryBreakpoint=false;
    else
        settings.eventEntryBreakpoint=true;
}

void SettingsDialog::on_chkDllEntry_stateChanged(int arg1)
{
    if(arg1==Qt::Unchecked)
        settings.eventDllEntry=false;
    else
        settings.eventDllEntry=true;
}

void SettingsDialog::on_chkThreadEntry_stateChanged(int arg1)
{
    if(arg1==Qt::Unchecked)
        settings.eventThreadEntry=false;
    else
        settings.eventThreadEntry=true;
}

void SettingsDialog::on_chkDllLoad_stateChanged(int arg1)
{
    if(arg1==Qt::Unchecked)
        settings.eventDllLoad=false;
    else
        settings.eventDllLoad=true;
}

void SettingsDialog::on_chkDllUnload_stateChanged(int arg1)
{
    if(arg1==Qt::Unchecked)
        settings.eventDllUnload=false;
    else
        settings.eventDllUnload=true;
}

void SettingsDialog::on_chkThreadStart_stateChanged(int arg1)
{
    if(arg1==Qt::Unchecked)
        settings.eventThreadStart=false;
    else
        settings.eventThreadStart=true;
}

void SettingsDialog::on_chkThreadEnd_stateChanged(int arg1)
{
    if(arg1==Qt::Unchecked)
        settings.eventThreadEnd=false;
    else
        settings.eventThreadEnd=true;
}

void SettingsDialog::on_chkDebugStrings_stateChanged(int arg1)
{
    if(arg1==Qt::Unchecked)
        settings.eventDebugStrings=false;
    else
        settings.eventDebugStrings=true;
}

void SettingsDialog::on_radioUnsigned_clicked()
{
    settings.engineCalcType=calc_unsigned;
}

void SettingsDialog::on_radioSigned_clicked()
{
    settings.engineCalcType=calc_signed;
}

void SettingsDialog::on_radioInt3Short_clicked()
{
    settings.engineBreakpointType=break_int3short;
}

void SettingsDialog::on_radioInt3Long_clicked()
{
    settings.engineBreakpointType=break_int3long;
}

void SettingsDialog::on_radioUd2_clicked()
{
    settings.engineBreakpointType=break_ud2;
}

void SettingsDialog::on_btnSave_clicked()
{
    SaveSettings();
    DbgSettingsUpdated();
}
