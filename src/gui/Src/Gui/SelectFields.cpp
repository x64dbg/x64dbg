#include "selectfields.h"
#include "ui_selectfields.h"

SelectFields::SelectFields(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::SelectFields)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint | Qt::MSWindowsFixedSizeDialogHint);
    setModal(true);
}

QListWidget* SelectFields::GetList()
{
    return ui->listWidget;
}

SelectFields::~SelectFields()
{
    delete ui;
}
