#include "selectfields.h"
#include "ui_selectfields.h"

SelectFields::SelectFields(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::SelectFields)
{
    ui->setupUi(this);
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
    setWindowFlags(Qt::Dialog | Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::MSWindowsFixedSizeDialogHint);
#endif
    setModal(true);
}

QListWidget* SelectFields::GetList(void)
{
    return ui->listWidget;
}

SelectFields::~SelectFields()
{
    delete ui;
}
