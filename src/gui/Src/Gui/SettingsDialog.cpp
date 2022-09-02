#include "SettingsDialog.h"
#include "ui_SettingsDialog.h"
#include <QMessageBox>
#include "Configuration.h"
#include "Bridge.h"
#include "ExceptionRangeDialog.h"
#include "MiscUtil.h"

SettingsDialog::SettingsDialog(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    //set window flags
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint | Qt::MSWindowsFixedSizeDialogHint);
    setModal(true);
    adjustSize();
    bTokenizerConfigUpdated = false;
    bDisableAutoCompleteUpdated = false;
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
    //Flush pending config changes
    Config()->save();

    //Defaults
    memset(&settings, 0, sizeof(SettingsStruct));
    settings.eventSystemBreakpoint = true;
    settings.eventTlsCallbacks = true;
    settings.eventEntryBreakpoint = true;
    settings.eventExitBreakpoint = false;
    settings.engineType = DebugEngineTitanEngine;
    settings.engineCalcType = calc_unsigned;
    settings.engineBreakpointType = break_int3short;
    settings.engineUndecorateSymbolNames = true;
    settings.engineEnableDebugPrivilege = true;
    settings.engineEnableSourceDebugging = false;
    settings.engineNoScriptTimeout = false;
    settings.engineIgnoreInconsistentBreakpoints = false;
    settings.engineNoWow64SingleStepWorkaround = false;
    settings.engineMaxTraceCount = 50000;
    settings.engineAnimateInterval = 50;
    settings.engineHardcoreThreadSwitchWarning = false;
    settings.engineVerboseExceptionLogging = true;
    settings.exceptionFilters = &realExceptionFilters;
    settings.disasmArgumentSpaces = false;
    settings.disasmHidePointerSizes = false;
    settings.disasmHideNormalSegments = false;
    settings.disasmMemorySpaces = false;
    settings.disasmUppercase = false;
    settings.disasmOnlyCipAutoComments = false;
    settings.disasmTabBetweenMnemonicAndArguments = false;
    settings.disasmNoCurrentModuleText = false;
    settings.disasm0xPrefixValues = false;
    settings.disasmNoBranchDisasmPreview = false;
    settings.disasmNoSourceLineAutoComments = false;
    settings.disasmAssembleOnDoubleClick = false;
    settings.disasmMaxModuleSize = -1;
    settings.guiNoForegroundWindow = true;
    settings.guiLoadSaveTabOrder = true;
    settings.guiDisableAutoComplete = false;
    settings.guiAutoFollowInStack = false;
    settings.guiHideSeasonalIcons = false;
    settings.guiEnableQtHighDpiScaling = true;

    //Events tab
    GetSettingBool("Events", "SystemBreakpoint", &settings.eventSystemBreakpoint);
    GetSettingBool("Events", "NtTerminateProcess", &settings.eventExitBreakpoint);
    GetSettingBool("Events", "TlsCallbacks", &settings.eventTlsCallbacks);
    GetSettingBool("Events", "TlsCallbacksSystem", &settings.eventTlsCallbacksSystem);
    GetSettingBool("Events", "EntryBreakpoint", &settings.eventEntryBreakpoint);
    GetSettingBool("Events", "DllEntry", &settings.eventDllEntry);
    GetSettingBool("Events", "DllEntrySystem", &settings.eventDllEntrySystem);
    GetSettingBool("Events", "ThreadEntry", &settings.eventThreadEntry);
    GetSettingBool("Events", "DllLoad", &settings.eventDllLoad);
    GetSettingBool("Events", "DllUnload", &settings.eventDllUnload);
    GetSettingBool("Events", "DllLoadSystem", &settings.eventDllLoadSystem);
    GetSettingBool("Events", "DllUnloadSystem", &settings.eventDllUnloadSystem);
    GetSettingBool("Events", "ThreadStart", &settings.eventThreadStart);
    GetSettingBool("Events", "ThreadEnd", &settings.eventThreadEnd);
    GetSettingBool("Events", "DebugStrings", &settings.eventDebugStrings);
    ui->chkSystemBreakpoint->setCheckState(bool2check(settings.eventSystemBreakpoint));
    ui->chkExitBreakpoint->setCheckState(bool2check(settings.eventExitBreakpoint));
    ui->chkTlsCallbacks->setCheckState(bool2check(settings.eventTlsCallbacks));
    ui->chkTlsCallbacksSystem->setCheckState(bool2check(settings.eventTlsCallbacksSystem));
    ui->chkEntryBreakpoint->setCheckState(bool2check(settings.eventEntryBreakpoint));
    ui->chkDllEntry->setCheckState(bool2check(settings.eventDllEntry));
    ui->chkDllEntrySystem->setCheckState(bool2check(settings.eventDllEntrySystem));
    ui->chkThreadEntry->setCheckState(bool2check(settings.eventThreadEntry));
    ui->chkDllLoad->setCheckState(bool2check(settings.eventDllLoad));
    ui->chkDllUnload->setCheckState(bool2check(settings.eventDllUnload));
    ui->chkDllLoadSystem->setCheckState(bool2check(settings.eventDllLoadSystem));
    ui->chkDllUnloadSystem->setCheckState(bool2check(settings.eventDllUnloadSystem));
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
    if(BridgeSettingGetUint("Engine", "DebugEngine", &cur))
    {
        settings.engineType = (DEBUG_ENGINE)cur;
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
    GetSettingBool("Engine", "EnableSourceDebugging", &settings.engineEnableSourceDebugging);
    GetSettingBool("Engine", "SaveDatabaseInProgramDirectory", &settings.engineSaveDatabaseInProgramDirectory);
    GetSettingBool("Engine", "DisableDatabaseCompression", &settings.engineDisableDatabaseCompression);
    GetSettingBool("Engine", "SkipInt3Stepping", &settings.engineSkipInt3Stepping);
    GetSettingBool("Engine", "NoScriptTimeout", &settings.engineNoScriptTimeout);
    GetSettingBool("Engine", "IgnoreInconsistentBreakpoints", &settings.engineIgnoreInconsistentBreakpoints);
    GetSettingBool("Engine", "HardcoreThreadSwitchWarning", &settings.engineHardcoreThreadSwitchWarning);
    GetSettingBool("Engine", "VerboseExceptionLogging", &settings.engineVerboseExceptionLogging);
    GetSettingBool("Engine", "NoWow64SingleStepWorkaround", &settings.engineNoWow64SingleStepWorkaround);
    if(BridgeSettingGetUint("Engine", "MaxTraceCount", &cur))
        settings.engineMaxTraceCount = int(cur);
    if(BridgeSettingGetUint("Engine", "AnimateInterval", &cur))
        settings.engineAnimateInterval = int(cur);
    switch(settings.engineCalcType)
    {
    case calc_signed:
        ui->radioSigned->setChecked(true);
        break;
    case calc_unsigned:
        ui->radioUnsigned->setChecked(true);
        break;
    }
    switch(settings.engineType)
    {
    case DebugEngineTitanEngine:
        ui->radioTitanEngine->setChecked(true);
        break;
    case DebugEngineGleeBug:
        ui->radioGleeBug->setChecked(true);
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
    ui->chkEnableSourceDebugging->setChecked(settings.engineEnableSourceDebugging);
    ui->chkSaveDatabaseInProgramDirectory->setChecked(settings.engineSaveDatabaseInProgramDirectory);
    ui->chkDisableDatabaseCompression->setChecked(settings.engineDisableDatabaseCompression);
    ui->chkSkipInt3Stepping->setChecked(settings.engineSkipInt3Stepping);
    ui->chkNoScriptTimeout->setChecked(settings.engineNoScriptTimeout);
    ui->chkIgnoreInconsistentBreakpoints->setChecked(settings.engineIgnoreInconsistentBreakpoints);
    ui->chkHardcoreThreadSwitchWarning->setChecked(settings.engineHardcoreThreadSwitchWarning);
    ui->chkVerboseExceptionLogging->setChecked(settings.engineVerboseExceptionLogging);
    ui->chkNoWow64SingleStepWorkaround->setChecked(settings.engineNoWow64SingleStepWorkaround);
    ui->spinMaxTraceCount->setValue(settings.engineMaxTraceCount);
    ui->spinAnimateInterval->setValue(settings.engineAnimateInterval);

    //Exceptions tab
    char exceptionRange[MAX_SETTING_SIZE] = "";
    bool unknownExceptionsFilterAdded = false;
    if(BridgeSettingGet("Exceptions", "IgnoreRange", exceptionRange))
    {
        QStringList ranges = QString(exceptionRange).split(QString(","), QString::SkipEmptyParts);
        for(int i = 0; i < ranges.size(); i++)
        {
            const QString & entry = ranges.at(i);
            unsigned long start;
            unsigned long end;
            if(!entry.contains("debug") && // check for old ignore format
                    sscanf_s(entry.toUtf8().constData(), "%08X-%08X", &start, &end) == 2 && start <= end)
            {
                ExceptionFilter newFilter;
                newFilter.range.start = start;
                newFilter.range.end = end;
                // Default settings for an ignore entry
                newFilter.breakOn = ExceptionBreakOn::SecondChance;
                newFilter.logException = true;
                newFilter.handledBy = ExceptionHandledBy::Debuggee;
                AddExceptionFilterToList(newFilter);
            }
            else if(entry.contains("debug") && // new filter format
                    sscanf_s(entry.toUtf8().constData(), "%08X-%08X", &start, &end) == 2 && start <= end)
            {
                ExceptionFilter newFilter;
                newFilter.range.start = start;
                newFilter.range.end = end;
                newFilter.breakOn = entry.contains("first") ? ExceptionBreakOn::FirstChance : entry.contains("second") ? ExceptionBreakOn::SecondChance : ExceptionBreakOn::DoNotBreak;
                newFilter.logException = !entry.contains("nolog");
                newFilter.handledBy = entry.contains("debugger") ? ExceptionHandledBy::Debugger : ExceptionHandledBy::Debuggee;
                AddExceptionFilterToList(newFilter);
                if(newFilter.range.start == 0 && newFilter.range.start == newFilter.range.end)
                    unknownExceptionsFilterAdded = true;
            }
        }
    }
    if(!unknownExceptionsFilterAdded) // add a default filter for unknown exceptions if it was not yet present in settings
    {
        ExceptionFilter unknownExceptionsFilter;
        unknownExceptionsFilter.range.start = unknownExceptionsFilter.range.end = 0;
        unknownExceptionsFilter.breakOn = ExceptionBreakOn::FirstChance;
        unknownExceptionsFilter.logException = true;
        unknownExceptionsFilter.handledBy = ExceptionHandledBy::Debuggee;
        AddExceptionFilterToList(unknownExceptionsFilter);
    }

    //Disasm tab
    GetSettingBool("Disassembler", "ArgumentSpaces", &settings.disasmArgumentSpaces);
    GetSettingBool("Disassembler", "HidePointerSizes", &settings.disasmHidePointerSizes);
    GetSettingBool("Disassembler", "HideNormalSegments", &settings.disasmHideNormalSegments);
    GetSettingBool("Disassembler", "MemorySpaces", &settings.disasmMemorySpaces);
    GetSettingBool("Disassembler", "Uppercase", &settings.disasmUppercase);
    GetSettingBool("Disassembler", "OnlyCipAutoComments", &settings.disasmOnlyCipAutoComments);
    GetSettingBool("Disassembler", "TabbedMnemonic", &settings.disasmTabBetweenMnemonicAndArguments);
    GetSettingBool("Disassembler", "NoHighlightOperands", &settings.disasmNoHighlightOperands);
    GetSettingBool("Disassembler", "PermanentHighlightingMode", &settings.disasmPermanentHighlightingMode);
    GetSettingBool("Disassembler", "NoCurrentModuleText", &settings.disasmNoCurrentModuleText);
    GetSettingBool("Disassembler", "0xPrefixValues", &settings.disasm0xPrefixValues);
    GetSettingBool("Disassembler", "NoBranchDisasmPreview", &settings.disasmNoBranchDisasmPreview);
    GetSettingBool("Disassembler", "NoSourceLineAutoComments", &settings.disasmNoSourceLineAutoComments);
    GetSettingBool("Disassembler", "AssembleOnDoubleClick", &settings.disasmAssembleOnDoubleClick);
    if(BridgeSettingGetUint("Disassembler", "MaxModuleSize", &cur))
        settings.disasmMaxModuleSize = int(cur);
    ui->chkArgumentSpaces->setChecked(settings.disasmArgumentSpaces);
    ui->chkHidePointerSizes->setChecked(settings.disasmHidePointerSizes);
    ui->chkHideNormalSegments->setChecked(settings.disasmHideNormalSegments);
    ui->chkMemorySpaces->setChecked(settings.disasmMemorySpaces);
    ui->chkUppercase->setChecked(settings.disasmUppercase);
    ui->chkOnlyCipAutoComments->setChecked(settings.disasmOnlyCipAutoComments);
    ui->chkTabBetweenMnemonicAndArguments->setChecked(settings.disasmTabBetweenMnemonicAndArguments);
    ui->chkNoHighlightOperands->setChecked(settings.disasmNoHighlightOperands);
    ui->chkPermanentHighlightingMode->setChecked(settings.disasmPermanentHighlightingMode);
    ui->chkNoCurrentModuleText->setChecked(settings.disasmNoCurrentModuleText);
    ui->chk0xPrefixValues->setChecked(settings.disasm0xPrefixValues);
    ui->chkNoBranchDisasmPreview->setChecked(settings.disasmNoBranchDisasmPreview);
    ui->chkNoSourceLinesAutoComments->setChecked(settings.disasmNoSourceLineAutoComments);
    ui->chkDoubleClickAssemble->setChecked(settings.disasmAssembleOnDoubleClick);
    ui->spinMaximumModuleNameSize->setValue(settings.disasmMaxModuleSize);

    //Gui tab
    GetSettingBool("Gui", "FpuRegistersLittleEndian", &settings.guiFpuRegistersLittleEndian);
    GetSettingBool("Gui", "SaveColumnOrder", &settings.guiSaveColumnOrder);
    GetSettingBool("Gui", "NoCloseDialog", &settings.guiNoCloseDialog);
    GetSettingBool("Gui", "PidTidInHex", &settings.guiPidTidInHex);
    GetSettingBool("Gui", "SidebarWatchLabels", &settings.guiSidebarWatchLabels);
    GetSettingBool("Gui", "NoForegroundWindow", &settings.guiNoForegroundWindow);
    GetSettingBool("Gui", "LoadSaveTabOrder", &settings.guiLoadSaveTabOrder);
    GetSettingBool("Gui", "ShowGraphRva", &settings.guiShowGraphRva);
    GetSettingBool("Gui", "GraphZoomMode", &settings.guiGraphZoomMode);
    GetSettingBool("Gui", "ShowExitConfirmation", &settings.guiShowExitConfirmation);
    GetSettingBool("Gui", "DisableAutoComplete", &settings.guiDisableAutoComplete);
    GetSettingBool("Gui", "AutoFollowInStack", &settings.guiAutoFollowInStack);
    GetSettingBool("Gui", "NoSeasons", &settings.guiHideSeasonalIcons);
    GetSettingBool("Gui", "EnableQtHighDpiScaling", &settings.guiEnableQtHighDpiScaling);
    ui->chkFpuRegistersLittleEndian->setChecked(settings.guiFpuRegistersLittleEndian);
    ui->chkSaveColumnOrder->setChecked(settings.guiSaveColumnOrder);
    ui->chkNoCloseDialog->setChecked(settings.guiNoCloseDialog);
    ui->chkPidTidInHex->setChecked(settings.guiPidTidInHex);
    ui->chkSidebarWatchLabels->setChecked(settings.guiSidebarWatchLabels);
    ui->chkNoForegroundWindow->setChecked(settings.guiNoForegroundWindow);
    ui->chkSaveLoadTabOrder->setChecked(settings.guiLoadSaveTabOrder);
    ui->chkShowGraphRva->setChecked(settings.guiShowGraphRva);
    ui->chkGraphZoomMode->setChecked(settings.guiGraphZoomMode);
    ui->chkShowExitConfirmation->setChecked(settings.guiShowExitConfirmation);
    ui->chkDisableAutoComplete->setChecked(settings.guiDisableAutoComplete);
    ui->chkAutoFollowInStack->setChecked(settings.guiAutoFollowInStack);
    ui->chkHideSeasonalIcons->setChecked(settings.guiHideSeasonalIcons);
    ui->chkHideSeasonalIcons->setVisible(isSeasonal());
    ui->chkQtHighDpiScaling->setChecked(settings.guiEnableQtHighDpiScaling);

    //Misc tab
    if(DbgFunctions()->GetJit)
    {
        char jit_entry[MAX_SETTING_SIZE] = "";
        char jit_def_entry[MAX_SETTING_SIZE] = "";
        bool isx64 = true;
#ifndef _WIN64
        isx64 = false;
#endif
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

        if(!BridgeIsProcessElevated())
        {
            ui->chkSetJIT->setDisabled(true);
            ui->lblAdminWarning->setText(QString(tr("<font color=\"red\"><b>Warning</b></font>: Run the debugger as Admin to enable JIT.")));
        }
        else
            ui->lblAdminWarning->setText("");
    }
    char setting[MAX_SETTING_SIZE] = "";
    if(BridgeSettingGet("Symbols", "DefaultStore", setting))
        ui->editSymbolStore->setText(QString(setting));
    else
    {
        QString defaultStore("https://msdl.microsoft.com/download/symbols");
        ui->editSymbolStore->setText(defaultStore);
        BridgeSettingSet("Symbols", "DefaultStore", defaultStore.toUtf8().constData());
    }
    if(BridgeSettingGet("Symbols", "CachePath", setting))
        ui->editSymbolCache->setText(QString(setting));
    if(BridgeSettingGet("Misc", "HelpOnSymbolicNameUrl", setting))
        ui->editHelpOnSymbolicNameUrl->setText(QString(setting));

    bJitOld = settings.miscSetJIT;

    GetSettingBool("Misc", "Utf16LogRedirect", &settings.miscUtf16LogRedirect);
    GetSettingBool("Misc", "UseLocalHelpFile", &settings.miscUseLocalHelpFile);
    GetSettingBool("Misc", "QueryProcessCookie", &settings.miscQueryProcessCookie);
    GetSettingBool("Misc", "QueryWorkingSet", &settings.miscQueryWorkingSet);
    GetSettingBool("Misc", "TransparentExceptionStepping", &settings.miscTransparentExceptionStepping);
    ui->chkUtf16LogRedirect->setChecked(settings.miscUtf16LogRedirect);
    ui->chkUseLocalHelpFile->setChecked(settings.miscUseLocalHelpFile);
    ui->chkQueryProcessCookie->setChecked(settings.miscQueryProcessCookie);
    ui->chkQueryWorkingSet->setChecked(settings.miscQueryWorkingSet);
    ui->chkTransparentExceptionStepping->setChecked(settings.miscTransparentExceptionStepping);
}

void SettingsDialog::SaveSettings()
{
    //Events tab
    BridgeSettingSetUint("Events", "SystemBreakpoint", settings.eventSystemBreakpoint);
    BridgeSettingSetUint("Events", "NtTerminateProcess", settings.eventExitBreakpoint);
    BridgeSettingSetUint("Events", "TlsCallbacks", settings.eventTlsCallbacks);
    BridgeSettingSetUint("Events", "TlsCallbacksSystem", settings.eventTlsCallbacksSystem);
    BridgeSettingSetUint("Events", "EntryBreakpoint", settings.eventEntryBreakpoint);
    BridgeSettingSetUint("Events", "DllEntry", settings.eventDllEntry);
    BridgeSettingSetUint("Events", "DllEntrySystem", settings.eventDllEntrySystem);
    BridgeSettingSetUint("Events", "ThreadEntry", settings.eventThreadEntry);
    BridgeSettingSetUint("Events", "DllLoad", settings.eventDllLoad);
    BridgeSettingSetUint("Events", "DllUnload", settings.eventDllUnload);
    BridgeSettingSetUint("Events", "DllLoadSystem", settings.eventDllLoadSystem);
    BridgeSettingSetUint("Events", "DllUnloadSystem", settings.eventDllUnloadSystem);
    BridgeSettingSetUint("Events", "ThreadStart", settings.eventThreadStart);
    BridgeSettingSetUint("Events", "ThreadEnd", settings.eventThreadEnd);
    BridgeSettingSetUint("Events", "DebugStrings", settings.eventDebugStrings);

    //Engine tab
    BridgeSettingSetUint("Engine", "CalculationType", settings.engineCalcType);
    BridgeSettingSetUint("Engine", "DebugEngine", settings.engineType);
    BridgeSettingSetUint("Engine", "BreakpointType", settings.engineBreakpointType);
    BridgeSettingSetUint("Engine", "UndecorateSymbolNames", settings.engineUndecorateSymbolNames);
    BridgeSettingSetUint("Engine", "EnableDebugPrivilege", settings.engineEnableDebugPrivilege);
    BridgeSettingSetUint("Engine", "EnableSourceDebugging", settings.engineEnableSourceDebugging);
    BridgeSettingSetUint("Engine", "SaveDatabaseInProgramDirectory", settings.engineSaveDatabaseInProgramDirectory);
    BridgeSettingSetUint("Engine", "DisableDatabaseCompression", settings.engineDisableDatabaseCompression);
    BridgeSettingSetUint("Engine", "SkipInt3Stepping", settings.engineSkipInt3Stepping);
    BridgeSettingSetUint("Engine", "NoScriptTimeout", settings.engineNoScriptTimeout);
    BridgeSettingSetUint("Engine", "IgnoreInconsistentBreakpoints", settings.engineIgnoreInconsistentBreakpoints);
    BridgeSettingSetUint("Engine", "MaxTraceCount", settings.engineMaxTraceCount);
    BridgeSettingSetUint("Engine", "AnimateInterval", settings.engineAnimateInterval);
    BridgeSettingSetUint("Engine", "VerboseExceptionLogging", settings.engineVerboseExceptionLogging);
    BridgeSettingSetUint("Engine", "HardcoreThreadSwitchWarning", settings.engineHardcoreThreadSwitchWarning);
    BridgeSettingSetUint("Engine", "NoWow64SingleStepWorkaround", settings.engineNoWow64SingleStepWorkaround);

    //Exceptions tab
    QString exceptionRange = "";
    for(int i = 0; i < settings.exceptionFilters->size(); i++)
    {
        const ExceptionFilter & filter = settings.exceptionFilters->at(i);
        exceptionRange.append(QString().asprintf("%.8X-%.8X:%s:%s:%s,",
                              filter.range.start, filter.range.end,
                              (filter.breakOn == ExceptionBreakOn::FirstChance ? "first" : filter.breakOn == ExceptionBreakOn::SecondChance ? "second" : "nobreak"),
                              (filter.logException ? "log" : "nolog"),
                              (filter.handledBy == ExceptionHandledBy::Debugger ? "debugger" : "debuggee")));
    }
    exceptionRange.chop(1); //remove last comma
    if(exceptionRange.size())
        BridgeSettingSet("Exceptions", "IgnoreRange", exceptionRange.toUtf8().constData());
    else
        BridgeSettingSet("Exceptions", "IgnoreRange", "");

    //Disasm tab
    BridgeSettingSetUint("Disassembler", "ArgumentSpaces", settings.disasmArgumentSpaces);
    BridgeSettingSetUint("Disassembler", "HidePointerSizes", settings.disasmHidePointerSizes);
    BridgeSettingSetUint("Disassembler", "HideNormalSegments", settings.disasmHideNormalSegments);
    BridgeSettingSetUint("Disassembler", "MemorySpaces", settings.disasmMemorySpaces);
    BridgeSettingSetUint("Disassembler", "Uppercase", settings.disasmUppercase);
    BridgeSettingSetUint("Disassembler", "OnlyCipAutoComments", settings.disasmOnlyCipAutoComments);
    BridgeSettingSetUint("Disassembler", "TabbedMnemonic", settings.disasmTabBetweenMnemonicAndArguments);
    BridgeSettingSetUint("Disassembler", "NoHighlightOperands", settings.disasmNoHighlightOperands);
    BridgeSettingSetUint("Disassembler", "PermanentHighlightingMode", settings.disasmPermanentHighlightingMode);
    BridgeSettingSetUint("Disassembler", "NoCurrentModuleText", settings.disasmNoCurrentModuleText);
    BridgeSettingSetUint("Disassembler", "0xPrefixValues", settings.disasm0xPrefixValues);
    BridgeSettingSetUint("Disassembler", "NoBranchDisasmPreview", settings.disasmNoBranchDisasmPreview);
    BridgeSettingSetUint("Disassembler", "NoSourceLineAutoComments", settings.disasmNoSourceLineAutoComments);
    BridgeSettingSetUint("Disassembler", "AssembleOnDoubleClick", settings.disasmAssembleOnDoubleClick);
    BridgeSettingSetUint("Disassembler", "MaxModuleSize", settings.disasmMaxModuleSize);

    //Gui tab
    BridgeSettingSetUint("Gui", "FpuRegistersLittleEndian", settings.guiFpuRegistersLittleEndian);
    BridgeSettingSetUint("Gui", "SaveColumnOrder", settings.guiSaveColumnOrder);
    BridgeSettingSetUint("Gui", "NoCloseDialog", settings.guiNoCloseDialog);
    BridgeSettingSetUint("Gui", "PidTidInHex", settings.guiPidTidInHex);
    BridgeSettingSetUint("Gui", "SidebarWatchLabels", settings.guiSidebarWatchLabels);
    BridgeSettingSetUint("Gui", "NoForegroundWindow", settings.guiNoForegroundWindow);
    BridgeSettingSetUint("Gui", "LoadSaveTabOrder", settings.guiLoadSaveTabOrder);
    BridgeSettingSetUint("Gui", "ShowGraphRva", settings.guiShowGraphRva);
    BridgeSettingSetUint("Gui", "GraphZoomMode", settings.guiGraphZoomMode);
    BridgeSettingSetUint("Gui", "ShowExitConfirmation", settings.guiShowExitConfirmation);
    BridgeSettingSetUint("Gui", "DisableAutoComplete", settings.guiDisableAutoComplete);
    BridgeSettingSetUint("Gui", "AutoFollowInStack", settings.guiAutoFollowInStack);
    BridgeSettingSetUint("Gui", "NoSeasons", settings.guiHideSeasonalIcons);
    BridgeSettingSetUint("Gui", "EnableQtHighDpiScaling", settings.guiEnableQtHighDpiScaling);

    //Misc tab
    if(DbgFunctions()->GetJit)
    {
        if(bJitOld != settings.miscSetJIT)
        {
            if(settings.miscSetJIT)
            {
                // Since Windows 10 WER will not trigger the JIT debugger at all without this
                DbgCmdExec("setjitauto on");
                DbgCmdExec("setjit oldsave");
            }
            else
                DbgCmdExec("setjit restore");
        }
    }
    if(settings.miscSymbolStore)
        BridgeSettingSet("Symbols", "DefaultStore", ui->editSymbolStore->text().toUtf8().constData());
    if(settings.miscSymbolCache)
        BridgeSettingSet("Symbols", "CachePath", ui->editSymbolCache->text().toUtf8().constData());
    BridgeSettingSet("Misc", "HelpOnSymbolicNameUrl", ui->editHelpOnSymbolicNameUrl->text().toUtf8().constData());
    BridgeSettingSetUint("Misc", "Utf16LogRedirect", settings.miscUtf16LogRedirect);
    BridgeSettingSetUint("Misc", "UseLocalHelpFile", settings.miscUseLocalHelpFile);
    BridgeSettingSetUint("Misc", "QueryProcessCookie", settings.miscQueryProcessCookie);
    BridgeSettingSetUint("Misc", "QueryWorkingSet", settings.miscQueryWorkingSet);
    BridgeSettingSetUint("Misc", "TransparentExceptionStepping", settings.miscTransparentExceptionStepping);

    BridgeSettingFlush();
    Config()->load();
    if(bTokenizerConfigUpdated)
    {
        emit Config()->tokenizerConfigUpdated();
        bTokenizerConfigUpdated = false;
    }
    if(bDisableAutoCompleteUpdated)
    {
        emit Config()->disableAutoCompleteUpdated();
        bDisableAutoCompleteUpdated = false;
    }
    if(bGuiOptionsUpdated)
    {
        emit Config()->guiOptionsUpdated();
        bGuiOptionsUpdated = false;
    }
    DbgSettingsUpdated();
    GuiUpdateAllViews();
}

void SettingsDialog::AddExceptionFilterToList(ExceptionFilter filter)
{
    //check range
    unsigned long start = filter.range.start;
    unsigned long end = filter.range.end;

    for(int i = settings.exceptionFilters->size() - 1; i > 0; i--)
    {
        unsigned long curStart = settings.exceptionFilters->at(i).range.start;
        unsigned long curEnd = settings.exceptionFilters->at(i).range.end;
        if(curStart <= end && curEnd >= start) //ranges overlap
        {
            if(curStart < start) //extend range to the left
                start = curStart;
            if(curEnd > end) //extend range to the right
                end = curEnd;
            settings.exceptionFilters->erase(settings.exceptionFilters->begin() + i); //remove old range
        }
    }
    filter.range.start = start;
    filter.range.end = end;
    settings.exceptionFilters->push_back(filter);
    UpdateExceptionListWidget();
}

void SettingsDialog::OnExceptionFilterSelectionChanged(QListWidgetItem* selected)
{
    QModelIndexList indexes = ui->listExceptions->selectionModel()->selectedIndexes();
    if(!indexes.size() && !selected) // no selection
        return;
    int row;
    if(!indexes.size())
        row = ui->listExceptions->row(selected);
    else
        row = indexes.at(0).row();
    if(row < 0 || row >= settings.exceptionFilters->count())
        return;

    const ExceptionFilter & filter = settings.exceptionFilters->at(row);
    if(filter.breakOn == ExceptionBreakOn::FirstChance)
        ui->radioFirstChance->setChecked(true);
    else if(filter.breakOn == ExceptionBreakOn::SecondChance)
        ui->radioSecondChance->setChecked(true);
    else
        ui->radioDoNotBreak->setChecked(true);
    if(filter.handledBy == ExceptionHandledBy::Debugger)
        ui->radioHandledByDebugger->setChecked(true);
    else
        ui->radioHandledByDebuggee->setChecked(true);
    ui->chkLogException->setChecked(filter.logException);

    if(filter.range.start == 0 && filter.range.start == filter.range.end) // disallow deleting the 'unknown exceptions' filter
        ui->btnDeleteRange->setEnabled(false);
    else
        ui->btnDeleteRange->setEnabled(true);
}

void SettingsDialog::OnCurrentExceptionFilterSettingsChanged()
{
    QModelIndexList indexes = ui->listExceptions->selectionModel()->selectedIndexes();
    if(!indexes.size()) // no selection
        return;
    int row = indexes.at(0).row();
    if(row < 0 || row >= settings.exceptionFilters->count())
        return;

    ExceptionFilter filter = settings.exceptionFilters->at(row);
    if(ui->radioFirstChance->isChecked())
        filter.breakOn = ExceptionBreakOn::FirstChance;
    else if(ui->radioSecondChance->isChecked())
        filter.breakOn = ExceptionBreakOn::SecondChance;
    else
        filter.breakOn = ExceptionBreakOn::DoNotBreak;
    filter.logException = ui->chkLogException->isChecked();
    if(ui->radioHandledByDebugger->isChecked())
        filter.handledBy = ExceptionHandledBy::Debugger;
    else
        filter.handledBy = ExceptionHandledBy::Debuggee;

    settings.exceptionFilters->erase(settings.exceptionFilters->begin() + row);
    settings.exceptionFilters->push_back(filter);
    qSort(settings.exceptionFilters->begin(), settings.exceptionFilters->end(), ExceptionFilterLess());
}

void SettingsDialog::UpdateExceptionListWidget()
{
    qSort(settings.exceptionFilters->begin(), settings.exceptionFilters->end(), ExceptionFilterLess());
    ui->listExceptions->clear();

    if(exceptionNames.empty() && DbgFunctions()->EnumExceptions)
    {
        BridgeList<CONSTANTINFO> exceptions;
        DbgFunctions()->EnumExceptions(&exceptions);
        for(int i = 0; i < exceptions.Count(); i++)
        {
            exceptionNames.insert({exceptions[i].value, exceptions[i].name});
        }
    }

    for(int i = 0; i < settings.exceptionFilters->size(); i++)
    {
        const ExceptionFilter & filter = settings.exceptionFilters->at(i);
        if(filter.range.start == 0 && filter.range.start == filter.range.end)
            ui->listExceptions->addItem(QString("Unknown exceptions"));
        else
        {
            const bool bSingleItemRange = filter.range.start == filter.range.end;
            if(!bSingleItemRange)
            {
                ui->listExceptions->addItem(QString().asprintf("%.8X-%.8X", filter.range.start, filter.range.end));
            }
            else
            {
                auto found = exceptionNames.find(filter.range.start);
                if(found == exceptionNames.end())
                    ui->listExceptions->addItem(QString().asprintf("%.8X", filter.range.start));
                else
                    ui->listExceptions->addItem(QString().asprintf("%.8X\n  %s", filter.range.start, found->second));
            }
        }
    }
}

void SettingsDialog::setLastException(unsigned int exceptionCode)
{
    lastException = exceptionCode;
}

void SettingsDialog::on_btnSave_clicked()
{
    SaveSettings();
    GuiAddStatusBarMessage(QString("%1\n").arg(tr("Settings saved!")).toUtf8().constData());
}

void SettingsDialog::on_chkSystemBreakpoint_stateChanged(int arg1)
{
    settings.eventSystemBreakpoint = arg1 != Qt::Unchecked;
}

void SettingsDialog::on_chkExitBreakpoint_stateChanged(int arg1)
{
    settings.eventExitBreakpoint = arg1 != Qt::Unchecked;
}

void SettingsDialog::on_chkTlsCallbacks_stateChanged(int arg1)
{
    settings.eventTlsCallbacks = arg1 != Qt::Unchecked;
}

void SettingsDialog::on_chkTlsCallbacksSystem_stateChanged(int arg1)
{
    settings.eventTlsCallbacksSystem = arg1 != Qt::Unchecked;
}

void SettingsDialog::on_chkEntryBreakpoint_stateChanged(int arg1)
{
    settings.eventEntryBreakpoint = arg1 != Qt::Unchecked;
}

void SettingsDialog::on_chkDllEntry_stateChanged(int arg1)
{
    settings.eventDllEntry = arg1 != Qt::Unchecked;
}

void SettingsDialog::on_chkDllEntrySystem_stateChanged(int arg1)
{
    settings.eventDllEntrySystem = arg1 != Qt::Unchecked;
}

void SettingsDialog::on_chkThreadEntry_stateChanged(int arg1)
{
    settings.eventThreadEntry = arg1 != Qt::Unchecked;
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
                SimpleWarningBox(this, tr("ERROR NOT FOUND OLD JIT"), tr("NOT FOUND OLD JIT ENTRY STORED, USE SETJIT COMMAND"));
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
    settings.eventDllLoad = arg1 != Qt::Unchecked;
}

void SettingsDialog::on_chkDllUnload_stateChanged(int arg1)
{
    settings.eventDllUnload = arg1 != Qt::Unchecked;
}

void SettingsDialog::on_chkDllLoadSystem_stateChanged(int arg1)
{
    settings.eventDllLoadSystem = arg1 != Qt::Unchecked;
}

void SettingsDialog::on_chkDllUnloadSystem_stateChanged(int arg1)
{
    settings.eventDllUnloadSystem = arg1 != Qt::Unchecked;
}

void SettingsDialog::on_chkThreadStart_stateChanged(int arg1)
{
    settings.eventThreadStart = arg1 != Qt::Unchecked;
}

void SettingsDialog::on_chkThreadEnd_stateChanged(int arg1)
{
    settings.eventThreadEnd = arg1 != Qt::Unchecked;
}

void SettingsDialog::on_chkDebugStrings_stateChanged(int arg1)
{
    settings.eventDebugStrings = arg1 != Qt::Unchecked;
}

void SettingsDialog::on_radioUnsigned_clicked()
{
    settings.engineCalcType = calc_unsigned;
}

void SettingsDialog::on_radioSigned_clicked()
{
    settings.engineCalcType = calc_signed;
}

void SettingsDialog::on_radioTitanEngine_clicked()
{
    settings.engineType = DebugEngineTitanEngine;
}

void SettingsDialog::on_radioGleeBug_clicked()
{
    settings.engineType = DebugEngineGleeBug;
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
    if(arg1 == Qt::Unchecked) //wtf stupid shit
        settings.engineEnableDebugPrivilege = false;
    else
        settings.engineEnableDebugPrivilege = true;
}

void SettingsDialog::on_chkHardcoreThreadSwitchWarning_toggled(bool checked)
{
    settings.engineHardcoreThreadSwitchWarning = checked;
}

void SettingsDialog::on_chkVerboseExceptionLogging_toggled(bool checked)
{
    settings.engineVerboseExceptionLogging = checked;
}

void SettingsDialog::on_chkEnableSourceDebugging_stateChanged(int arg1)
{
    settings.engineEnableSourceDebugging = arg1 == Qt::Checked;
}

void SettingsDialog::on_chkDisableDatabaseCompression_stateChanged(int arg1)
{
    settings.engineDisableDatabaseCompression = arg1 == Qt::Checked;
}

void SettingsDialog::on_chkSaveDatabaseInProgramDirectory_stateChanged(int arg1)
{
    settings.engineSaveDatabaseInProgramDirectory = arg1 == Qt::Checked;
}
void SettingsDialog::on_btnIgnoreRange_clicked()
{
    ExceptionRangeDialog exceptionRange(this);
    if(exceptionRange.exec() != QDialog::Accepted)
        return;

    ExceptionFilter filter;
    filter.range.start = exceptionRange.rangeStart;
    filter.range.end = exceptionRange.rangeEnd;
    filter.breakOn = ExceptionBreakOn::SecondChance;
    filter.logException = true;
    filter.handledBy = ExceptionHandledBy::Debuggee;
    AddExceptionFilterToList(filter);
}

void SettingsDialog::on_btnDeleteRange_clicked()
{
    QModelIndexList indexes = ui->listExceptions->selectionModel()->selectedIndexes();
    if(!indexes.size()) //no selection
        return;

    settings.exceptionFilters->erase(settings.exceptionFilters->begin() + indexes.at(0).row());
    UpdateExceptionListWidget();
}

void SettingsDialog::on_btnIgnoreLast_clicked()
{
    QMessageBox msg(QMessageBox::Question, tr("Question"), QString().sprintf(tr("Are you sure you want to add %.8X?").toUtf8().constData(), lastException));
    msg.setWindowIcon(DIcon("question"));
    msg.setParent(this, Qt::Dialog);
    msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
    msg.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
    msg.setDefaultButton(QMessageBox::Yes);
    if(msg.exec() != QMessageBox::Yes)
        return;

    ExceptionFilter filter;
    filter.range.start = lastException;
    filter.range.end = lastException;
    filter.breakOn = ExceptionBreakOn::SecondChance;
    filter.logException = true;
    filter.handledBy = ExceptionHandledBy::Debuggee;
    AddExceptionFilterToList(filter);
}

// Shortcut to ignore all first chance exceptions and don't log
void SettingsDialog::on_btnIgnoreFirst_clicked()
{
    for(int i = 0; i < settings.exceptionFilters->size(); i++)
    {
        ExceptionFilter & filter = (*settings.exceptionFilters)[i];
        if(filter.range.start == 0 && filter.range.end == 0)
        {
            filter.breakOn = ExceptionBreakOn::SecondChance;
            filter.handledBy = ExceptionHandledBy::Debuggee;
            filter.logException = false;
        }
    }
    UpdateExceptionListWidget();
}

void SettingsDialog::on_listExceptions_currentItemChanged(QListWidgetItem* current, QListWidgetItem*)
{
    OnExceptionFilterSelectionChanged(current);
}

void SettingsDialog::on_listExceptions_itemClicked(QListWidgetItem* item)
{
    OnExceptionFilterSelectionChanged(item);
}

void SettingsDialog::on_radioFirstChance_clicked()
{
    OnCurrentExceptionFilterSettingsChanged();
}

void SettingsDialog::on_radioSecondChance_clicked()
{
    OnCurrentExceptionFilterSettingsChanged();
}

void SettingsDialog::on_radioDoNotBreak_clicked()
{
    OnCurrentExceptionFilterSettingsChanged();
}

void SettingsDialog::on_chkLogException_stateChanged(int arg1)
{
    Q_UNUSED(arg1);
    OnCurrentExceptionFilterSettingsChanged();
}

void SettingsDialog::on_radioHandledByDebugger_clicked()
{
    OnCurrentExceptionFilterSettingsChanged();
}

void SettingsDialog::on_radioHandledByDebuggee_clicked()
{
    OnCurrentExceptionFilterSettingsChanged();
}

void SettingsDialog::on_chkArgumentSpaces_stateChanged(int arg1)
{
    bTokenizerConfigUpdated = true;
    settings.disasmArgumentSpaces = arg1 != Qt::Unchecked;
}

void SettingsDialog::on_chkHidePointerSizes_stateChanged(int arg1)
{
    bTokenizerConfigUpdated = true;
    settings.disasmHidePointerSizes = arg1 != Qt::Unchecked;
}

void SettingsDialog::on_chkHideNormalSegments_stateChanged(int arg1)
{
    bTokenizerConfigUpdated = true;
    settings.disasmHideNormalSegments = arg1 != Qt::Unchecked;
}

void SettingsDialog::on_chkMemorySpaces_stateChanged(int arg1)
{
    bTokenizerConfigUpdated = true;
    settings.disasmMemorySpaces = arg1 != Qt::Unchecked;
}

void SettingsDialog::on_chkUppercase_stateChanged(int arg1)
{
    bTokenizerConfigUpdated = true;
    settings.disasmUppercase = arg1 != Qt::Unchecked;
}

void SettingsDialog::on_chkOnlyCipAutoComments_stateChanged(int arg1)
{
    settings.disasmOnlyCipAutoComments = arg1 != Qt::Unchecked;
}

void SettingsDialog::on_chkTabBetweenMnemonicAndArguments_stateChanged(int arg1)
{
    bTokenizerConfigUpdated = true;
    settings.disasmTabBetweenMnemonicAndArguments = arg1 == Qt::Checked;
}

void SettingsDialog::on_editSymbolStore_textEdited(const QString & arg1)
{
    Q_UNUSED(arg1);
    settings.miscSymbolStore = true;
}

void SettingsDialog::on_editSymbolCache_textEdited(const QString & arg1)
{
    Q_UNUSED(arg1);
    settings.miscSymbolCache = true;
}

void SettingsDialog::on_chkSaveLoadTabOrder_stateChanged(int arg1)
{
    settings.guiLoadSaveTabOrder = arg1 != Qt::Unchecked;
    emit chkSaveLoadTabOrderStateChanged((bool)arg1);
}

void SettingsDialog::on_chkFpuRegistersLittleEndian_stateChanged(int arg1)
{
    settings.guiFpuRegistersLittleEndian = arg1 != Qt::Unchecked;
}

void SettingsDialog::on_chkSaveColumnOrder_stateChanged(int arg1)
{
    settings.guiSaveColumnOrder = arg1 != Qt::Unchecked;
}

void SettingsDialog::on_chkNoCloseDialog_toggled(bool checked)
{
    settings.guiNoCloseDialog = checked;
}

void SettingsDialog::on_chkAutoFollowInStack_toggled(bool checked)
{
    bGuiOptionsUpdated = true;
    settings.guiAutoFollowInStack = checked;
}

void SettingsDialog::on_chkSkipInt3Stepping_toggled(bool checked)
{
    settings.engineSkipInt3Stepping = checked;
}

void SettingsDialog::on_chkPidTidInHex_clicked(bool checked)
{
    settings.guiPidTidInHex = checked;
}

void SettingsDialog::on_chkNoScriptTimeout_stateChanged(int arg1)
{
    settings.engineNoScriptTimeout = arg1 != Qt::Unchecked;
}

void SettingsDialog::on_chkSidebarWatchLabels_stateChanged(int arg1)
{
    settings.guiSidebarWatchLabels = arg1 != Qt::Unchecked;
}

void SettingsDialog::on_chkIgnoreInconsistentBreakpoints_toggled(bool checked)
{
    settings.engineIgnoreInconsistentBreakpoints = checked;
}

void SettingsDialog::on_chkNoForegroundWindow_toggled(bool checked)
{
    settings.guiNoForegroundWindow = checked;
}

void SettingsDialog::on_spinMaxTraceCount_valueChanged(int arg1)
{
    settings.engineMaxTraceCount = arg1;
}

void SettingsDialog::on_spinAnimateInterval_valueChanged(int arg1)
{
    settings.engineAnimateInterval = arg1;
}

void SettingsDialog::on_chkNoHighlightOperands_toggled(bool checked)
{
    bTokenizerConfigUpdated = true;
    settings.disasmNoHighlightOperands = checked;
}

void SettingsDialog::on_chkUtf16LogRedirect_toggled(bool checked)
{
    settings.miscUtf16LogRedirect = checked;
}

void SettingsDialog::on_chkPermanentHighlightingMode_toggled(bool checked)
{
    bTokenizerConfigUpdated = true;
    settings.disasmPermanentHighlightingMode = checked;
}

void SettingsDialog::on_chkNoWow64SingleStepWorkaround_toggled(bool checked)
{
    settings.engineNoWow64SingleStepWorkaround = checked;
}

void SettingsDialog::on_chkNoCurrentModuleText_toggled(bool checked)
{
    bTokenizerConfigUpdated = true;
    settings.disasmNoCurrentModuleText = checked;
}

void SettingsDialog::on_chk0xPrefixValues_toggled(bool checked)
{
    bTokenizerConfigUpdated = true;
    settings.disasm0xPrefixValues = checked;
}

void SettingsDialog::on_chkNoBranchDisasmPreview_toggled(bool checked)
{
    bGuiOptionsUpdated = true;
    settings.disasmNoBranchDisasmPreview = checked;
}

void SettingsDialog::on_chkNoSourceLinesAutoComments_toggled(bool checked)
{
    settings.disasmNoSourceLineAutoComments = checked;
}

void SettingsDialog::on_chkDoubleClickAssemble_toggled(bool checked)
{
    settings.disasmAssembleOnDoubleClick = checked;
}

void SettingsDialog::on_spinMaximumModuleNameSize_valueChanged(int arg1)
{
    settings.disasmMaxModuleSize = arg1;
}

void SettingsDialog::on_chkShowGraphRva_toggled(bool checked)
{
    bTokenizerConfigUpdated = true;
    settings.guiShowGraphRva = checked;
}

void SettingsDialog::on_chkGraphZoomMode_toggled(bool checked)
{
    bTokenizerConfigUpdated = true;
    settings.guiGraphZoomMode = checked;
}

void SettingsDialog::on_chkShowExitConfirmation_toggled(bool checked)
{
    settings.guiShowExitConfirmation = checked;
}

void SettingsDialog::on_chkDisableAutoComplete_toggled(bool checked)
{
    settings.guiDisableAutoComplete = checked;
    bDisableAutoCompleteUpdated = true;
}

void SettingsDialog::on_chkHideSeasonalIcons_toggled(bool checked)
{
    settings.guiHideSeasonalIcons = checked;
}

void SettingsDialog::on_chkUseLocalHelpFile_toggled(bool checked)
{
    settings.miscUseLocalHelpFile = checked;
}

void SettingsDialog::on_chkQueryProcessCookie_toggled(bool checked)
{
    settings.miscQueryProcessCookie = checked;
}

void SettingsDialog::on_chkQueryWorkingSet_toggled(bool checked)
{
    settings.miscQueryWorkingSet = checked;
}

void SettingsDialog::on_chkTransparentExceptionStepping_toggled(bool checked)
{
    settings.miscTransparentExceptionStepping = checked;
}

void SettingsDialog::on_chkQtHighDpiScaling_toggled(bool checked)
{
    settings.guiEnableQtHighDpiScaling = checked;
}

