#include "PageMemoryRights.h"
#include "ui_PageMemoryRights.h"
#include <string>

PageMemoryRights::PageMemoryRights(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::PageMemoryRights)
{
    ui->setupUi(this);
    //set window flags
    setModal(true);
    setWindowFlags(Qt::Dialog | Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::MSWindowsFixedSizeDialogHint);
}

PageMemoryRights::~PageMemoryRights()
{
    delete ui;
}

void PageMemoryRights::RunAddrSize(uint_t addrin, uint_t sizein)
{
    addr = addrin;
    size = sizein;

    int charwidth = QFontMetrics(this->font()).width(QChar(' '));

    //addColumnAt(8 + charwidth * 2 * sizeof(uint_t), "ADDR", false, "Address"); //addr
    QTableWidget* tableWidget = ui->pagetableWidget;
    tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

    uint_t nr_pages = size / 1000;
    tableWidget->setColumnCount(2);
    tableWidget->setRowCount(nr_pages);

    tableWidget->setHorizontalHeaderItem(0, new QTableWidgetItem(QString("ADDR")));
    tableWidget->setHorizontalHeaderItem(1, new QTableWidgetItem(QString("RIGHTS")));

    for(uint_t i = 0; i < nr_pages; i++)
    {
        tableWidget->setItem(i, 0, new QTableWidgetItem(QString("%1").arg(addr + (i * 1000), sizeof(uint_t) * 2, 16, QChar('0')).toUpper()));


    }


    QModelIndex idx = (ui->pagetableWidget->model()->index(0, 0));
    ui->pagetableWidget->selectionModel()->select(idx, QItemSelectionModel::Select);
    idx = (ui->pagetableWidget->model()->index(0, 1));
    ui->pagetableWidget->selectionModel()->select(idx, QItemSelectionModel::Select);

    ui->radioFullaccess->setChecked(true);
    exec();

}



void PageMemoryRights::on_btnSelectall_clicked()
{
    for(int i = 0; i < ui->pagetableWidget->rowCount(); i++)
    {
        for(int j = 0; j < ui->pagetableWidget->columnCount(); j++)
        {
            QModelIndex idx = (ui->pagetableWidget->model()->index(i, j));
            ui->pagetableWidget->selectionModel()->select(idx, QItemSelectionModel::Select);
        }
    }
}

void PageMemoryRights::on_btnDeselectall_clicked()
{
    QModelIndexList indexList = ui->pagetableWidget->selectionModel()->selectedIndexes();
    foreach(QModelIndex index, indexList)
    {
        ui->pagetableWidget->selectionModel()->select(index, QItemSelectionModel::Deselect);

    }
}
