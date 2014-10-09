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
}

CloseDialog::~CloseDialog()
{
    delete ui;
}

void CloseDialog::closeEvent(QCloseEvent* event)
{
    event->ignore();
}
