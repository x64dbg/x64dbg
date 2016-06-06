#include "ColumnReorderDialog.h"
#include "ui_ColumnReorderDialog.h"
#include <QMessageBox>

ColumnReorderDialog::ColumnReorderDialog(AbstractTableView* parent) :
    QDialog(parent),
    mParent(parent),
    ui(new Ui::ColumnReorderDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint | Qt::MSWindowsFixedSizeDialogHint);
    for(int j = 0; j < parent->getColumnCount(); j++)
    {
        int i = parent->mColumnOrder[j];
        if(parent->getColumnHidden(i))
        {
            ui->listAvailable->addItem(parent->getColTitle(i));
            ui->listAvailable->item(ui->listAvailable->count() - 1)->setData(1, QVariant(j));
        }
        else
        {
            ui->listDisplayed->addItem(parent->getColTitle(i));
            ui->listDisplayed->item(ui->listDisplayed->count() - 1)->setData(1, QVariant(j));
        }
    }
    if(ui->listAvailable->count() == 0)
    {
        ui->addAllButton->setEnabled(false);
        ui->addButton->setEnabled(false);
    }
}

ColumnReorderDialog::~ColumnReorderDialog()
{
    delete ui;
}

void ColumnReorderDialog::on_okButton_clicked()
{
    int i = 0;
    if(ui->listDisplayed->count() == 0)
    {
        QMessageBox msg(QMessageBox::Warning, tr("Error"), tr("There isn't anything to display yet!"));
        msg.exec();
        return;
    }
    for(i = 0; i < ui->listDisplayed->count(); i++)
    {
        int col = ui->listDisplayed->item(i)->data(1).toInt();
        mParent->mColumnOrder[i] = col;
        mParent->setColumnHidden(col, false);
    }
    for(int j = 0; j < ui->listAvailable->count(); j++, i++)
    {
        int col = ui->listAvailable->item(j)->data(1).toInt();
        mParent->mColumnOrder[i] = col;
        mParent->setColumnHidden(col, true);
    }
    this->done(QDialog::Accepted);
}

void ColumnReorderDialog::on_cancelButton_clicked()
{
    this->done(QDialog::Rejected);
}

void ColumnReorderDialog::on_addButton_clicked()
{
    if(ui->listAvailable->selectedItems().empty())
        return;
    auto selected = ui->listAvailable->selectedItems();
    for(auto i : selected)
    {
        ui->listDisplayed->addItem(i->text());
        ui->listDisplayed->item(ui->listDisplayed->count() - 1)->setData(1, i->data(1));
        delete i;
    }
    if(ui->listAvailable->count() == 0)
    {
        ui->addAllButton->setEnabled(false);
        ui->addButton->setEnabled(false);
    }
}

void ColumnReorderDialog::on_addAllButton_clicked()
{
    for(int i = 0; i < ui->listAvailable->count(); i++)
    {
        ui->listDisplayed->addItem(ui->listAvailable->item(i)->text());
        ui->listDisplayed->item(ui->listDisplayed->count() - 1)->setData(1, ui->listAvailable->item(i)->data(1));
    }
    ui->listAvailable->clear();
    ui->addAllButton->setEnabled(false);
    ui->addButton->setEnabled(false);
}

void ColumnReorderDialog::on_upButton_clicked()
{
    if(ui->listDisplayed->selectedItems().empty())
        return;
    auto i = ui->listDisplayed->currentIndex().row();
    if(i != 0)
    {
        auto prevItem = ui->listDisplayed->item(i - 1);
        auto currentItem = ui->listDisplayed->item(i);
        QString text = prevItem->text();
        auto data = prevItem->data(1);
        prevItem->setText(currentItem->text());
        prevItem->setData(1, currentItem->data(1));
        currentItem->setText(text);
        currentItem->setData(1, data);
        ui->listDisplayed->setCurrentRow(i - 1);
    }
}

void ColumnReorderDialog::on_downButton_clicked()
{
    if(ui->listDisplayed->selectedItems().empty())
        return;
    auto i = ui->listDisplayed->currentIndex().row();
    if(i != ui->listDisplayed->count() - 1)
    {
        auto nextItem = ui->listDisplayed->item(i + 1);
        auto currentItem = ui->listDisplayed->item(i);
        QString text = nextItem->text();
        auto data = nextItem->data(1);
        nextItem->setText(currentItem->text());
        nextItem->setData(1, currentItem->data(1));
        currentItem->setText(text);
        currentItem->setData(1, data);
        ui->listDisplayed->setCurrentRow(i + 1);
    }
}

void ColumnReorderDialog::on_hideButton_clicked()
{
    if(ui->listDisplayed->selectedItems().empty())
        return;
    auto currentItem = ui->listDisplayed->currentItem();
    ui->listAvailable->addItem(currentItem->text());
    ui->listAvailable->item(ui->listAvailable->count() - 1)->setData(1, currentItem->data(1));
    delete currentItem;
    ui->addAllButton->setEnabled(true);
    ui->addButton->setEnabled(true);
}
