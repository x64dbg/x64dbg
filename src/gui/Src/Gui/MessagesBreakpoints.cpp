#include "MessagesBreakpoints.h"
#include "ui_MessagesBreakpoints.h"

MessagesBreakpoints::MessagesBreakpoints(MsgBreakpointData pbpData, QWidget* parent) :
    QDialog(parent),
    ui(new Ui::MessagesBreakpoints)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint | Qt::MSWindowsFixedSizeDialogHint);
    setModal(true);
    bpData = pbpData;

    int index = 0;

    filterMessages =
    {
        "ACM_", "BFFM_", "BM_", "CB_", "CBEM_", "CCM_", "CDM_", "CTL3D_", "DDM_", "DL_",
        "DM_", "DTM_", "EM_", "HDM_", "HKM_", "IE_", "IPM_", "LB_", "LVM_", "MCIWNDM_",
        "MCM_", "MN_", "MSG_", "NIN_", "OCM_", "PBM_", "PGM_", "PSM_", "RB_", "SB_",
        "SBM_", "SM_", "STM_", "TAPI_", "TB_", "TBM_", "TCM_", "TTM_", "TV_", "TVM_",
        "UDM_", "UM_", "WIZ_", "WLX_", "WM_"
    };

    BridgeList<CONSTANTINFO> constants;
    DbgFunctions()->EnumConstants(&constants);

    for(int i = 0; i < constants.Count(); i++)
    {
        foreach(QString filter, filterMessages)
        {
            if(QString(constants[i].name).startsWith(filter))
            {
                messages.insert(constants[i].value, constants[i].name);
                ui->cboxMessages->addItem(constants[i].name);
                break;
            }
        }
    }
    for(int i = 0; i < ui->cboxMessages->count(); i++)
        if(ui->cboxMessages->itemText(i) == "WM_COMMAND")
            index = i;
    ui->cboxMessages->setCurrentIndex(index);
}

MessagesBreakpoints::~MessagesBreakpoints()
{
    delete ui;
}

void MessagesBreakpoints::on_btnOk_clicked()
{
    duint procVA;
    duint wndHandle;

    if(!DbgFunctions()->ValFromString(bpData.wndHandle.toUtf8().constData(), &wndHandle) ||
            !DbgFunctions()->ValFromString(bpData.procVA.toUtf8().constData(), &procVA))
        return;

    if(!DbgMemIsValidReadPtr(procVA) || !IsWindow((HWND)wndHandle))
        return;

    BPXTYPE wBpType = DbgGetBpxTypeAt(procVA);
    if(wBpType == bp_none)
        DbgCmdExec(QString("bp 0x%1").arg(bpData.procVA).toUtf8().constData());

    QString bpCondCmd = QString("bpcnd 0x%1, \"arg.get(1) == 0x%2").arg(bpData.procVA).arg(messages.key(ui->cboxMessages->currentText()), 1, 16);
    bpCondCmd.append(ui->rbtnBreakCurrent->isChecked() ? QString(" && arg.get(0) == 0x%1\"").arg(bpData.wndHandle) : "\"");
    DbgCmdExec(bpCondCmd.toUtf8().constData());
}
