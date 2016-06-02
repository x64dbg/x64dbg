#include "CloseDialog.h"
#include "ui_CloseDialog.h"
#include "MiscUtil.h"

CloseDialog::CloseDialog(QWidget* parent) : QDialog(parent), ui(new Ui::CloseDialog)
{
    ui->setupUi(this);
    setModal(true);
    setWindowFlags(windowFlags() & ~(Qt::WindowContextHelpButtonHint | Qt::WindowCloseButtonHint) | Qt::MSWindowsFixedSizeDialogHint);
    setFixedSize(this->size()); //fixed size
    SetApplicationIcon(QDialog::winId());
    bCanClose = false;
}

CloseDialog::~CloseDialog()
{
    delete ui;
}

void CloseDialog::allowClose()
{
    bCanClose = true;
}

void CloseDialog::closeEvent(QCloseEvent* event)
{
    if(bCanClose)
        event->accept();
    else
        event->ignore();
}
