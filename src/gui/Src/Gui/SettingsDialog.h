#pragma once

#include <QDialog>
#include <QListWidgetItem>
#include "Imports.h"

namespace Ui
{
    class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr);
    ~SettingsDialog();
    void SaveSettings();
    unsigned int lastException;

signals:
    void chkSaveLoadTabOrderStateChanged(bool state);

private slots:
    //Manual slots
    void setLastException(unsigned int exceptionCode);
    //General
    void on_btnSave_clicked();
    //Event tab
    void on_chkSystemBreakpoint_stateChanged(int arg1);
    void on_chkExitBreakpoint_stateChanged(int arg1);
    void on_chkTlsCallbacks_stateChanged(int arg1);
    void on_chkTlsCallbacksSystem_stateChanged(int arg1);
    void on_chkEntryBreakpoint_stateChanged(int arg1);
    void on_chkDllEntry_stateChanged(int arg1);
    void on_chkDllEntrySystem_stateChanged(int arg1);
    void on_chkThreadEntry_stateChanged(int arg1);
    void on_chkDllLoad_stateChanged(int arg1);
    void on_chkDllUnload_stateChanged(int arg1);
    void on_chkDllLoadSystem_stateChanged(int arg1);
    void on_chkDllUnloadSystem_stateChanged(int arg1);
    void on_chkThreadStart_stateChanged(int arg1);
    void on_chkThreadEnd_stateChanged(int arg1);
    void on_chkThreadNameSet_stateChanged(int arg1);
    void on_chkDebugStrings_stateChanged(int arg1);
    //Engine tab
    void on_radioUnsigned_clicked();
    void on_radioSigned_clicked();
    void on_radioTitanEngine_clicked();
    void on_radioGleeBug_clicked();
    void on_radioInt3Short_clicked();
    void on_radioInt3Long_clicked();
    void on_radioUd2_clicked();
    void on_chkUndecorateSymbolNames_stateChanged(int arg1);
    void on_chkEnableDebugPrivilege_stateChanged(int arg1);
    void on_chkEnableSourceDebugging_stateChanged(int arg1);
    void on_chkDisableDatabaseCompression_stateChanged(int arg1);
    void on_chkSaveDatabaseInProgramDirectory_stateChanged(int arg1);
    void on_chkSkipInt3Stepping_toggled(bool checked);
    void on_chkNoScriptTimeout_stateChanged(int arg1);
    void on_chkIgnoreInconsistentBreakpoints_toggled(bool checked);
    void on_chkHardcoreThreadSwitchWarning_toggled(bool checked);
    void on_chkVerboseExceptionLogging_toggled(bool checked);
    void on_chkNoWow64SingleStepWorkaround_toggled(bool checked);
    void on_chkDisableAslr_toggled(bool checked);
    void on_spinMaxTraceCount_valueChanged(int arg1);
    void on_spinAnimateInterval_valueChanged(int arg1);
    //Exception tab
    void on_btnIgnoreRange_clicked();
    void on_btnDeleteRange_clicked();
    void on_btnIgnoreLast_clicked();
    void on_btnIgnoreFirst_clicked();
    void on_listExceptions_currentItemChanged(QListWidgetItem* current, QListWidgetItem* previous);
    void on_listExceptions_itemClicked(QListWidgetItem* item);
    void on_radioFirstChance_clicked();
    void on_radioSecondChance_clicked();
    void on_radioDoNotBreak_clicked();
    void on_chkLogException_stateChanged(int arg1);
    void on_radioHandledByDebugger_clicked();
    void on_radioHandledByDebuggee_clicked();
    //Disasm tab
    void on_chkArgumentSpaces_stateChanged(int arg1);
    void on_chkHidePointerSizes_stateChanged(int arg1);
    void on_chkHideNormalSegments_stateChanged(int arg1);
    void on_chkMemorySpaces_stateChanged(int arg1);
    void on_chkUppercase_stateChanged(int arg1);
    void on_chkOnlyCipAutoComments_stateChanged(int arg1);
    void on_chkTabBetweenMnemonicAndArguments_stateChanged(int arg1);
    void on_chkNoHighlightOperands_toggled(bool checked);
    void on_chkNoCurrentModuleText_toggled(bool checked);
    void on_chkPermanentHighlightingMode_toggled(bool checked);
    void on_chk0xPrefixValues_toggled(bool checked);
    void on_chkNoBranchDisasmPreview_toggled(bool checked);
    void on_chkNoSourceLinesAutoComments_toggled(bool checked);
    void on_chkDoubleClickAssemble_toggled(bool checked);
    void on_spinMaximumModuleNameSize_valueChanged(int arg1);
    //Gui Tab
    void on_chkFpuRegistersLittleEndian_stateChanged(int arg1);
    void on_chkSaveColumnOrder_stateChanged(int arg1);
    void on_chkSaveLoadTabOrder_stateChanged(int arg1);
    void on_chkNoCloseDialog_toggled(bool checked);
    void on_chkPidTidInHex_clicked(bool checked);
    void on_chkSidebarWatchLabels_stateChanged(int arg1);
    void on_chkNoForegroundWindow_toggled(bool checked);
    void on_chkShowExitConfirmation_toggled(bool checked);
    void on_chkDisableAutoComplete_toggled(bool checked);
    void on_chkAutoFollowInStack_toggled(bool checked);
    void on_chkHideSeasonalIcons_toggled(bool checked);
    void on_chkQtHighDpiScaling_toggled(bool checked);
    void on_chkWindowLongPath_toggled(bool checked);
    void on_chkNoIcons_toggled(bool checked);
    void on_chkDisableTraceDump_toggled(bool checked);
    //Misc tab
    void on_chkSetJIT_stateChanged(int arg1);
    void on_editSymbolStore_textEdited(const QString & arg1);
    void on_editSymbolCache_textEdited(const QString & arg1);
    void on_chkUtf16LogRedirect_toggled(bool checked);
    void on_chkShowGraphRva_toggled(bool checked);
    void on_chkGraphZoomMode_toggled(bool checked);
    void on_chkUseLocalHelpFile_toggled(bool checked);
    void on_chkQueryProcessCookie_toggled(bool checked);
    void on_chkQueryWorkingSet_toggled(bool checked);
    void on_chkTransparentExceptionStepping_toggled(bool checked);

private:
    //enums
    enum CalcType
    {
        calc_signed = 0,
        calc_unsigned = 1
    };

    enum BreakpointType
    {
        break_int3short = 0,
        break_int3long = 1,
        break_ud2 = 2
    };

    enum class ExceptionBreakOn
    {
        FirstChance,
        SecondChance,
        DoNotBreak
    };

    enum class ExceptionHandledBy
    {
        Debugger,
        Debuggee
    };

    //structures
    struct RangeStruct
    {
        unsigned long start;
        unsigned long end;
    };

    struct ExceptionFilter
    {
        RangeStruct range;
        ExceptionBreakOn breakOn;
        bool logException;
        ExceptionHandledBy handledBy;
    };

    struct ExceptionFilterLess
    {
        bool operator()(const ExceptionFilter a, const ExceptionFilter b) const
        {
            return a.range.start < b.range.start;
        }
    };

    // Unfortunately there are multiple sources of truth for the defaults.
    // Some are in Configuration::Configuration and some in _exports.cpp
    // case DBG_SETTINGS_UPDATED
    struct SettingsStruct
    {
        //Event Tab
        bool eventSystemBreakpoint = true;
        bool eventExitBreakpoint = false;
        bool eventTlsCallbacks = true;
        bool eventTlsCallbacksSystem = true;
        bool eventEntryBreakpoint = true;
        bool eventDllEntry = false;
        bool eventDllEntrySystem = false;
        bool eventThreadEntry = false;
        bool eventDllLoad = false;
        bool eventDllUnload = false;
        bool eventDllLoadSystem = false;
        bool eventDllUnloadSystem = false;
        bool eventThreadStart = false;
        bool eventThreadEnd = false;
        bool eventThreadNameSet = false;
        bool eventDebugStrings = false;
        //Engine Tab
        CalcType engineCalcType = calc_unsigned;
        DEBUG_ENGINE engineType = DebugEngineTitanEngine;
        BreakpointType engineBreakpointType = break_int3short;
        bool engineUndecorateSymbolNames = true;
        bool engineEnableDebugPrivilege = true;
        bool engineEnableSourceDebugging = false;
        bool engineSaveDatabaseInProgramDirectory = false;
        bool engineDisableDatabaseCompression = false;
        bool engineSkipInt3Stepping = false;
        bool engineNoScriptTimeout = false;
        bool engineIgnoreInconsistentBreakpoints = false;
        bool engineHardcoreThreadSwitchWarning = false;
        bool engineVerboseExceptionLogging = true;
        bool engineNoWow64SingleStepWorkaround = false;
        bool engineDisableAslr = false;
        int engineMaxTraceCount = 50000;
        int engineAnimateInterval = 50;
        //Exception Tab
        QList<ExceptionFilter>* exceptionFilters = nullptr;
        //Disasm Tab
        bool disasmArgumentSpaces = false;
        bool disasmMemorySpaces = false;
        bool disasmHidePointerSizes = false;
        bool disasmHideNormalSegments = false;
        bool disasmUppercase = false;
        bool disasmOnlyCipAutoComments = false;
        bool disasmTabBetweenMnemonicAndArguments = false;
        bool disasmNoHighlightOperands;
        bool disasmNoCurrentModuleText = false;
        bool disasmPermanentHighlightingMode;
        bool disasm0xPrefixValues = false;
        bool disasmNoBranchDisasmPreview = false;
        bool disasmNoSourceLineAutoComments = false;
        bool disasmAssembleOnDoubleClick = false;
        int disasmMaxModuleSize = -1;
        //Gui Tab
        bool guiFpuRegistersLittleEndian = false;
        bool guiSaveColumnOrder = false;
        bool guiNoCloseDialog = false;
        bool guiPidTidInHex = false;
        bool guiSidebarWatchLabels = false;
        bool guiNoForegroundWindow = true;
        bool guiLoadSaveTabOrder = true;
        bool guiShowGraphRva = false;
        bool guiGraphZoomMode = true;
        bool guiShowExitConfirmation = true;
        bool guiDisableAutoComplete = false;
        bool guiAutoFollowInStack = false;
        bool guiHideSeasonalIcons = false;
        bool guiEnableQtHighDpiScaling = true;
        bool guiEnableWindowLongPath = false;
        bool guiNoIcons = false;
        bool guiDisableTraceDump = false;
        //Misc Tab
        bool miscSetJIT = false;
        bool miscSymbolStore = false;
        bool miscSymbolCache = false;
        bool miscUtf16LogRedirect = false;
        bool miscUseLocalHelpFile = false;
        bool miscQueryProcessCookie = false;
        bool miscQueryWorkingSet = false;
        bool miscTransparentExceptionStepping = true;
    };

    //variables
    Ui::SettingsDialog* ui;
    SettingsStruct settings;
    QList<ExceptionFilter> realExceptionFilters;
    std::unordered_map<duint, const char*> exceptionNames;
    bool bJitOld;
    bool bGuiOptionsUpdated;
    bool bTokenizerConfigUpdated;
    bool bDisableAutoCompleteUpdated;

    //functions
    void GetSettingBool(const char* section, const char* name, bool* set);
    Qt::CheckState bool2check(bool checked);
    void LoadSettings();
    void AddExceptionFilterToList(ExceptionFilter filter);
    void OnExceptionFilterSelectionChanged(QListWidgetItem* selected);
    void OnCurrentExceptionFilterSettingsChanged();
    void UpdateExceptionListWidget();
};
