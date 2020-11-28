#include "CloseDialog.h"
#include "ui_CloseDialog.h"
#include "MiscUtil.h"
#include <QCloseEvent>

CloseDialog::CloseDialog(QWidget* parent) : QDialog(parent), ui(new Ui::CloseDialog)
{
    ui->setupUi(this);
    setModal(true);
    setWindowFlags(windowFlags() & ~(Qt::WindowContextHelpButtonHint | Qt::WindowCloseButtonHint) | Qt::MSWindowsFixedSizeDialogHint);
    SetApplicationIcon(QDialog::winId());
    bCanClose = false;
    // Resize window
    int requiredWidth = ui->label->fontMetrics().boundingRect(ui->label->text()).width();
    if(width() < requiredWidth)
        resize(requiredWidth, height());
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
