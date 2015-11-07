#include "CloseDialog.h"
#include "ui_CloseDialog.h"

CloseDialog::CloseDialog(QWidget* parent) : QDialog(parent), ui(new Ui::CloseDialog)
{
    ui->setupUi(this);
    setModal(true);
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
    setWindowFlags((Qt::Tool | Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::MSWindowsFixedSizeDialogHint) & ~Qt::WindowCloseButtonHint);
#endif
    setFixedSize(this->size()); //fixed size
    //setWindowFlags(((windowFlags() | Qt::CustomizeWindowHint) & ~Qt::WindowCloseButtonHint));
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
