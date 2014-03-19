#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include "Bridge.h"

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = 0);
    ~SettingsDialog();

private slots:
    void on_chkSystemBreakpoint_stateChanged(int arg1);
    void on_chkTlsCallbacks_stateChanged(int arg1);
    void on_chkEntryBreakpoint_stateChanged(int arg1);
    void on_chkDllEntry_stateChanged(int arg1);
    void on_chkThreadEntry_stateChanged(int arg1);
    void on_chkDllLoad_stateChanged(int arg1);
    void on_chkDllUnload_stateChanged(int arg1);
    void on_chkThreadStart_stateChanged(int arg1);
    void on_chkThreadEnd_stateChanged(int arg1);
    void on_chkDebugStrings_stateChanged(int arg1);
    void on_radioUnsigned_clicked();
    void on_radioSigned_clicked();
    void on_radioInt3Short_clicked();
    void on_radioInt3Long_clicked();
    void on_radioUd2_clicked();
    void on_btnSave_clicked();

private:
    //enums
    enum CalcType
    {
        calc_signed=0,
        calc_unsigned=1
    };

    enum BreakpointType
    {
        break_int3short=0,
        break_int3long=1,
        break_ud2=2
    };

    //structures
    struct SettingsStruct
    {
        //Event Tab
        bool eventSystemBreakpoint;
        bool eventTlsCallbacks;
        bool eventEntryBreakpoint;
        bool eventDllEntry;
        bool eventThreadEntry;
        bool eventDllLoad;
        bool eventDllUnload;
        bool eventThreadStart;
        bool eventThreadEnd;
        bool eventDebugStrings;
        //Engine Tab
        CalcType engineCalcType;
        BreakpointType engineBreakpointType;
        //Exception Tab
    };

    //variables
    Ui::SettingsDialog *ui;
    SettingsStruct settings;

    //functions
    void GetSettingBool(const char* section, const char* name, bool* set);
    Qt::CheckState bool2check(bool checked);
    void LoadSettings();
    void SaveSettings();
};

#endif // SETTINGSDIALOG_H
