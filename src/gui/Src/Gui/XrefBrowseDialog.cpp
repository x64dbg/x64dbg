#include "XrefBrowseDialog.h"
#include "ui_XrefBrowseDialog.h"
#include "StringUtil.h"

XrefBrowseDialog::XrefBrowseDialog(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::XrefBrowseDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint | Qt::MSWindowsFixedSizeDialogHint);
    setWindowIcon(DIcon("xrefs.png"));
    setModal(false);
}

void XrefBrowseDialog::setup(duint address, QString command)
{
    mCommand = command;
    mAddress = address;
    mPrevSelectionSize = 0;
    if(DbgXrefGet(address, &mXrefInfo))
    {
        char disasm[GUI_MAX_DISASSEMBLY_SIZE] = "";
        setWindowTitle(QString(tr("xrefs at %1")).arg(ToHexString(address)));
        for(duint i = 0; i < mXrefInfo.refcount; i++)
        {
            if(GuiGetDisassembly(mXrefInfo.references[i].addr, disasm))
                ui->listWidget->addItem(disasm);
            else
                ui->listWidget->addItem("???");
        }
        ui->listWidget->setCurrentRow(0);
    }
}

void XrefBrowseDialog::changeAddress(duint address)
{
    DbgCmdExec(QString("%1 %2").arg(mCommand, ToPtrString(address)).toUtf8().constData());
}

XrefBrowseDialog::~XrefBrowseDialog()
{
    delete ui;
    if(mXrefInfo.refcount)
        BridgeFree(mXrefInfo.references);
}

void XrefBrowseDialog::on_listWidget_itemDoubleClicked(QListWidgetItem*)
{
    accept();
}

void XrefBrowseDialog::on_listWidget_itemSelectionChanged()
{
    if(ui->listWidget->selectedItems().size() != mPrevSelectionSize)
    {
        duint address;
        if(mPrevSelectionSize == 0)
            address = mXrefInfo.references[ui->listWidget->currentRow()].addr;
        else
            address = mAddress;

        changeAddress(address);
    }
    mPrevSelectionSize = ui->listWidget->selectedItems().size();
}

void XrefBrowseDialog::on_listWidget_currentRowChanged(int row)
{
    if(ui->listWidget->selectedItems().size() != 0)
    {
        duint address = mXrefInfo.references[row].addr;
        changeAddress(address);
    }
}

void XrefBrowseDialog::on_XrefBrowseDialog_rejected()
{
    DbgCmdExec(QString("%1 %2").arg(mCommand, ToPtrString(mAddress)).toUtf8().constData());
}

void XrefBrowseDialog::on_listWidget_itemClicked(QListWidgetItem*)
{
    on_listWidget_currentRowChanged(ui->listWidget->currentRow());
}
