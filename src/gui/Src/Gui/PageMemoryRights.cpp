#include "Imports.h"
#include "PageMemoryRights.h"
#include "ui_PageMemoryRights.h"
#include "StringUtil.h"

PageMemoryRights::PageMemoryRights(QWidget* parent) : QDialog(parent), ui(new Ui::PageMemoryRights)
{
    ui->setupUi(this);
    //set window flags
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setModal(true);
    addr = 0;
    size = 0;
}

PageMemoryRights::~PageMemoryRights()
{
    delete ui;
}

void PageMemoryRights::RunAddrSize(duint addrin, duint sizein, QString pagetypein)
{
    addr = addrin;
    size = sizein;
    pagetype = pagetypein;

    QTableWidget* tableWidget = ui->pagetableWidget;
    tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    duint nr_pages = size / PAGE_SIZE;
    tableWidget->setColumnCount(2);
    tableWidget->setRowCount(nr_pages);
    tableWidget->setHorizontalHeaderItem(0, new QTableWidgetItem(QString(tr("Address"))));
    tableWidget->setHorizontalHeaderItem(1, new QTableWidgetItem(QString(tr("Rights"))));

    duint actual_addr;
    char rights[RIGHTS_STRING_SIZE];
    for(duint i = 0; i < nr_pages; i++)
    {
        actual_addr = addr + (i * PAGE_SIZE);
        tableWidget->setItem(i, 0, new QTableWidgetItem(ToPtrString(actual_addr)));
        if(DbgFunctions()->GetPageRights(actual_addr, rights))
            tableWidget->setItem(i, 1, new QTableWidgetItem(QString(rights)));
    }
    tableWidget->resizeColumnsToContents();

    QModelIndex idx = (ui->pagetableWidget->model()->index(0, 0));
    ui->pagetableWidget->selectionModel()->select(idx, QItemSelectionModel::Select);
    idx = (ui->pagetableWidget->model()->index(0, 1));
    ui->pagetableWidget->selectionModel()->select(idx, QItemSelectionModel::Select);

    ui->radioFullaccess->setChecked(true);
    ui->chkPageguard->setCheckable(true);
    exec();
}

void PageMemoryRights::on_btnSelectall_clicked()
{
    const auto rowCount = ui->pagetableWidget->rowCount();
    const auto columnCount = ui->pagetableWidget->columnCount();

    QModelIndex topLeft = ui->pagetableWidget->model()->index(0, 0);
    QModelIndex bottomRight = ui->pagetableWidget->model()->index(rowCount - 1, columnCount - 1);

    QItemSelection selection(topLeft, bottomRight);
    ui->pagetableWidget->selectionModel()->select(selection, QItemSelectionModel::Select);
}

void PageMemoryRights::on_btnDeselectall_clicked()
{
    QModelIndexList indexList = ui->pagetableWidget->selectionModel()->selectedIndexes();
    foreach(QModelIndex index, indexList)
    {
        ui->pagetableWidget->selectionModel()->select(index, QItemSelectionModel::Deselect);

    }
}

void PageMemoryRights::on_btnSetrights_clicked()
{
    duint actual_addr;
    QString rights;
    char newrights[RIGHTS_STRING_SIZE];
    bool one_right_changed = false;

    if(ui->radioExecute->isChecked())
        rights = "Execute";
    else if(ui->radioExecuteread->isChecked())
        rights = "ExecuteRead";
    else if(ui->radioNoaccess->isChecked())
        rights = "NoAccess";
    else if(ui->radioFullaccess ->isChecked())
        rights = "ExecuteReadWrite";
    else if(ui->radioReadonly->isChecked())
        rights = "ReadOnly";
    else if(ui->radioReadwrite->isChecked())
        rights = "ReadWrite";
    else if(ui->radioWritecopy->isChecked())
        rights = "WriteCopy";
    else if(ui->radioExecutewritecopy->isChecked())
        rights = "ExecuteWriteCopy";
    else
        return;

    if(ui->chkPageguard->isChecked())
        rights = "G" + rights;

    QModelIndexList indexList = ui->pagetableWidget->selectionModel()->selectedIndexes();
    foreach(QModelIndex index, indexList)
    {
#ifdef _WIN64
        actual_addr = ui->pagetableWidget->item(index.row(), 0)->text().toULongLong(0, 16);
#else //x86
        actual_addr = ui->pagetableWidget->item(index.row(), 0)->text().toULong(0, 16);
#endif //_WIN64
        if(DbgFunctions()->SetPageRights(actual_addr, (char*)rights.toUtf8().constData()))
        {
            one_right_changed = true;
            if(DbgFunctions()->GetPageRights(actual_addr, newrights))
                ui->pagetableWidget->setItem(index.row(), 1, new QTableWidgetItem(QString(newrights)));
        }
    }

    DbgFunctions()->MemUpdateMap();
    emit refreshMemoryMap();

    if(one_right_changed)
        ui->LnEdStatus->setText(tr("Pages Rights Changed to: ") + rights);
    else
        ui->LnEdStatus->setText(tr("Error setting rights, read the MSDN to learn the valid rights of: ") + pagetype);
}
