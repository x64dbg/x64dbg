#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

namespace Ui
{
    class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = 0);
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
    void on_chkTlsCallbacks_stateChanged(int arg1);
    void on_chkEntryBreakpoint_stateChanged(int arg1);
    void on_chkDllEntry_stateChanged(int arg1);
    void on_chkThreadEntry_stateChanged(int arg1);
    void on_chkAttachBreakpoint_stateChanged(int arg1);
    void on_chkDllLoad_stateChanged(int arg1);
    void on_chkDllUnload_stateChanged(int arg1);
    void on_chkThreadStart_stateChanged(int arg1);
    void on_chkThreadEnd_stateChanged(int arg1);
    void on_chkDebugStrings_stateChanged(int arg1);
    //Engine tab
    void on_radioUnsigned_clicked();
    void on_radioSigned_clicked();
    void on_radioInt3Short_clicked();
    void on_radioInt3Long_clicked();
    void on_radioUd2_clicked();
    void on_chkUndecorateSymbolNames_stateChanged(int arg1);
    void on_chkEnableDebugPrivilege_stateChanged(int arg1);
    void on_chkEnableSourceDebugging_stateChanged(int arg1);
    void on_chkDisableDatabaseCompression_stateChanged(int arg1);
    void on_chkSaveDatabaseInProgramDirectory_stateChanged(int arg1);
    void on_chkTraceRecordEnabledDuringTrace_stateChanged(int arg1);
    void on_chkSkipInt3Stepping_toggled(bool checked);
    void on_chkNoScriptTimeout_stateChanged(int arg1);
    void on_chkIgnoreInconsistentBreakpoints_toggled(bool checked);
    void on_chkHardcoreThreadSwitchWarning_toggled(bool checked);
    void on_chkVerboseExceptionLogging_toggled(bool checked);
    void on_chkNoWow64SingleStepWorkaround_toggled(bool checked);
    void on_spinMaxTraceCount_valueChanged(int arg1);
    void on_spinAnimateInterval_valueChanged(int arg1);
    //Exception tab
    void on_btnAddRange_clicked();
    void on_btnDeleteRange_clicked();
    void on_btnAddLast_clicked();
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
    //Misc tab
    void on_chkSetJIT_stateChanged(int arg1);
    void on_chkConfirmBeforeAtt_stateChanged(int arg1);
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

    //structures
    struct RangeStruct
    {
        unsigned long start;
        unsigned long end;
    };

    struct RangeStructLess
    {
        bool operator()(const RangeStruct a, const RangeStruct b) const
        {
            return a.start < b.start;
        }
    };

    struct SettingsStruct
    {
        //Event Tab
        bool eventSystemBreakpoint;
        bool eventTlsCallbacks;
        bool eventEntryBreakpoint;
        bool eventDllEntry;
        bool eventThreadEntry;
        bool eventAttachBreakpoint;
        bool eventDllLoad;
        bool eventDllUnload;
        bool eventThreadStart;
        bool eventThreadEnd;
        bool eventDebugStrings;
        //Engine Tab
        CalcType engineCalcType;
        BreakpointType engineBreakpointType;
        bool engineUndecorateSymbolNames;
        bool engineEnableDebugPrivilege;
        bool engineEnableSourceDebugging;
        bool engineSaveDatabaseInProgramDirectory;
        bool engineDisableDatabaseCompression;
        bool engineEnableTraceRecordDuringTrace;
        bool engineSkipInt3Stepping;
        bool engineNoScriptTimeout;
        bool engineIgnoreInconsistentBreakpoints;
        bool engineHardcoreThreadSwitchWarning;
        bool engineVerboseExceptionLogging;
        bool engineNoWow64SingleStepWorkaround;
        int engineMaxTraceCount;
        int engineAnimateInterval;
        //Exception Tab
        QList<RangeStruct>* exceptionRanges;
        //Disasm Tab
        bool disasmArgumentSpaces;
        bool disasmMemorySpaces;
        bool disasmHidePointerSizes;
        bool disasmHideNormalSegments;
        bool disasmUppercase;
        bool disasmOnlyCipAutoComments;
        bool disasmTabBetweenMnemonicAndArguments;
        bool disasmNoHighlightOperands;
        bool disasmNoCurrentModuleText;
        bool disasmPermanentHighlightingMode;
        bool disasm0xPrefixValues;
        bool disasmNoBranchDisasmPreview;
        bool disasmNoSourceLineAutoComments;
        int disasmMaxModuleSize;
        //Gui Tab
        bool guiFpuRegistersLittleEndian;
        bool guiSaveColumnOrder;
        bool guiNoCloseDialog;
        bool guiPidTidInHex;
        bool guiSidebarWatchLabels;
        bool guiNoForegroundWindow;
        bool guiLoadSaveTabOrder;
        bool guiShowGraphRva;
        bool guiGraphZoomMode;
        bool guiShowExitConfirmation;
        bool guiDisableAutoComplete;
        //Misc Tab
        bool miscSetJIT;
        bool miscSetJITAuto;
        bool miscSymbolStore;
        bool miscSymbolCache;
        bool miscUtf16LogRedirect;
        bool miscUseLocalHelpFile;
        bool miscQueryProcessCookie;
        bool miscQueryWorkingSet;
        bool miscTransparentExceptionStepping;
    };

    //variables
    Ui::SettingsDialog* ui;
    SettingsStruct settings;
    QList<RangeStruct> realExceptionRanges;
    bool bJitOld;
    bool bJitAutoOld;
    bool bGuiOptionsUpdated;
    bool bTokenizerConfigUpdated;
    bool bDisableAutoCompleteUpdated;

    //functions
    void GetSettingBool(const char* section, const char* name, bool* set);
    Qt::CheckState bool2check(bool checked);
    void LoadSettings();
    void AddRangeToList(RangeStruct range);
};

#endif // SETTINGSDIALOG_H
