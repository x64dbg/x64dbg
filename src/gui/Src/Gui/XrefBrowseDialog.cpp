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
        for(int i = 0; i < this->mXrefInfo.refcount; i++)
        {
            ui->listWidget->addItem(QString("%1").arg(this->mXrefInfo.references[i], 10, 16, QChar('0')));
        }
        connect(ui->listWidget, SIGNAL(currentRowChanged(int)), this, SLOT(on_listview_clicked(int)));
    }
    connect(this, SIGNAL(rejected()), this, SLOT(on_buttonCancel_clicked()));

}

void XrefBrowseDialog::on_buttonCancel_clicked()
{
    DbgCmdExec(QString().sprintf("disasm \"%p\"", this->mAddress).toUtf8().constData());
}

void XrefBrowseDialog::on_listview_clicked(int row)
{
    duint address = this->mXrefInfo.references[row];
    DbgCmdExec(QString().sprintf("disasm \"%p\"", address).toUtf8().constData());
}


XrefBrowseDialog::~XrefBrowseDialog()
{
    delete ui;
    if(this->mXrefInfo.refcount)
        BridgeFree(this->mXrefInfo.references);
}
