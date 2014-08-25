#include "CloseDialog.h"
#include "ui_CloseDialog.h"

CloseDialog::CloseDialog(QWidget* parent) : QDialog(parent), ui(new Ui::CloseDialog)
{
    ui->setupUi(this);
    setModal(true);
    setWindowFlags((Qt::Tool | Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::MSWindowsFixedSizeDialogHint) & ~Qt::WindowCloseButtonHint);
    setFixedSize(this->size()); //fixed size
    //setWindowFlags(((windowFlags() | Qt::CustomizeWindowHint) & ~Qt::WindowCloseButtonHint));
}

CloseDialog::~CloseDialog()
{
    delete ui;
}
