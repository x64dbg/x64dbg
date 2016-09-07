#include "VirtualModDialog.h"
#include "ui_VirtualModDialog.h"
#include "StringUtil.h"

VirtualModDialog::VirtualModDialog(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::VirtualModDialog)
{
    ui->setupUi(this);
    setModal(true);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint | Qt::MSWindowsFixedSizeDialogHint);
}

VirtualModDialog::~VirtualModDialog()
{
    delete ui;
}

bool VirtualModDialog::getData(QString & modname, duint & base, duint & size)
{
    modname = ui->editName->text();
    if(!modname.length())
        return false;
    bool ok = false;
    base = duint(ui->editBase->text().toLongLong(&ok, 16));
    if(!ok || !DbgMemIsValidReadPtr(base))
        return false;
    size = duint(ui->editSize->text().toLongLong(&ok, 16));
    if(!ok || ! size)
        return false;
    return true;
}

void VirtualModDialog::setData(const QString & modname, duint base, duint size)
{
    ui->editName->setText(modname);
    ui->editBase->setText(ToHexString(base));
    ui->editSize->setText(ToHexString(size));
}
