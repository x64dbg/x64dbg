#include "PageMemoryRights.h"
#include "ui_PageMemoryRights.h"

PageMemoryRights::PageMemoryRights(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::PageMemoryRights)
{
    ui->setupUi(this);
}

PageMemoryRights::~PageMemoryRights()
{
    delete ui;
}
