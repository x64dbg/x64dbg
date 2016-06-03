#include "XrefBrowseDialog.h"
#include "ui_XrefBrowseDialog.h"

XrefBrowseDialog::XrefBrowseDialog(QWidget* parent, duint address) :
    QDialog(parent),
    ui(new Ui::XrefBrowseDialog)
{
    ui->setupUi(this);
    this->mAddress = address;
    if(DbgXrefGet(address, &this->mXrefInfo))
    {
        this->setWindowTitle(QString(tr("All known jumps and calls to %1")).arg(address, 0, 16));
        for(int i = 0; i < this->mXrefInfo.refcount; i++)
        {
            ui->listWidget->addItem(QString(tr("%1 from %2")).arg(this->mXrefInfo.references[i].inst).arg(this->mXrefInfo.references[i].addr, 0, 16));
        }
        mPrevSelectionSize = 0;
        connect(ui->listWidget, SIGNAL(currentRowChanged(int)), this, SLOT(on_currentRow_changed(int)));
        connect(ui->listWidget, SIGNAL(itemSelectionChanged()), this, SLOT(on_selection_changed()));
        connect(ui->listWidget, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(on_item_DoubleClicked(QListWidgetItem*)));
        connect(this, SIGNAL(rejected()), this, SLOT(on_buttonCancel_clicked()));
    }
}

void XrefBrowseDialog::on_buttonCancel_clicked()
{
    DbgCmdExec(QString().sprintf("disasm \"%p\"", this->mAddress).toUtf8().constData());
}

void XrefBrowseDialog::on_currentRow_changed(int row)
{

    if(ui->listWidget->selectedItems().size() != 0)
    {
        duint address = this->mXrefInfo.references[row].addr;
        changeAddress(address);
    }
}

void XrefBrowseDialog::on_selection_changed()
{
    if(ui->listWidget->selectedItems().size() != mPrevSelectionSize)
    {
        duint address;
        if(mPrevSelectionSize == 0)
            address = this->mXrefInfo.references[ui->listWidget->currentRow()].addr;
        else
            address = mAddress;

        changeAddress(address);
    }
    mPrevSelectionSize = ui->listWidget->selectedItems().size();
}


void XrefBrowseDialog::on_item_DoubleClicked(QListWidgetItem* item)
{
    this->accept();
}

void XrefBrowseDialog::changeAddress(duint address)
{
    DbgCmdExec(QString().sprintf("disasm \"%p\"", address).toUtf8().constData());
}

XrefBrowseDialog::~XrefBrowseDialog()
{
    delete ui;
    if(this->mXrefInfo.refcount)
        BridgeFree(this->mXrefInfo.references);
}
