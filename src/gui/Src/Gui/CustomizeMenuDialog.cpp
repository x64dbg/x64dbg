#include "CustomizeMenuDialog.h"
#include "ui_CustomizeMenuDialog.h"
#include "MenuBuilder.h"
#include "Configuration.h"

CustomizeMenuDialog::CustomizeMenuDialog(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::CustomizeMenuDialog)
{
    ui->setupUi(this);
    for(const auto & i : Config()->NamedMenuBuilders)
    {
        QString viewName;
        const char* id = i.first->getId();
        if(strcmp(id, "CPUDisassembly") == 0)
            viewName = tr("Disassembler");
        else if(strcmp(id, "CPUDump") == 0)
            viewName = tr("Dump");
        else if(strcmp(id, "WatchView") == 0)
            viewName = tr("Watch");
        else if(strcmp(id, "CallStackView") == 0)
            viewName = tr("Call Stack");
        else if(strcmp(id, "ThreadView") == 0)
            viewName = tr("Threads");
        else
            continue;
        QTreeWidgetItem* parentItem = new QTreeWidgetItem(ui->treeWidget);
        parentItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        parentItem->setText(0, viewName);
        for(size_t j = 0; j < i.second; j++)
        {
            QString text = i.first->getText(j);
            if(!text.isEmpty())
            {
                QTreeWidgetItem* menuItem = new QTreeWidgetItem(parentItem, 0);
                menuItem->setText(0, text.replace(QChar('&'), ""));
                QString configString = QString("Menu%1Hidden%2").arg(i.first->getId()).arg(j);
                menuItem->setCheckState(0, Config()->getBool("Gui", configString) ? Qt::Checked : Qt::Unchecked);
                menuItem->setData(0, Qt::UserRole, QVariant(configString));
                menuItem->setFlags(Qt::ItemIsSelectable | Qt::ItemNeverHasChildren | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            }
        }
        ui->treeWidget->addTopLevelItem(parentItem);
    }
    connect(ui->btnOk, SIGNAL(clicked()), this, SLOT(onOk()));
    connect(ui->btnDisselectAll, SIGNAL(clicked()), this, SLOT(onDisselectAll()));
}

void CustomizeMenuDialog::onOk()
{
    for(int i = ui->treeWidget->topLevelItemCount(); i != 0; i--)
    {
        const QTreeWidgetItem* parentItem = ui->treeWidget->topLevelItem(i - 1);
        for(int j = parentItem->childCount(); j != 0; j--)
        {
            const QTreeWidgetItem* childItem = parentItem->child(j - 1);
            Config()->setBool("Gui", childItem->data(0, Qt::UserRole).toString(), childItem->checkState(0) == Qt::Checked);
        }
    }
    emit accept();
}

void CustomizeMenuDialog::onDisselectAll()
{
    for(int i = ui->treeWidget->topLevelItemCount(); i != 0; i--)
    {
        const QTreeWidgetItem* parentItem = ui->treeWidget->topLevelItem(i - 1);
        for(int j = parentItem->childCount(); j != 0; j--)
        {
            parentItem->child(j - 1)->setCheckState(0, Qt::Unchecked);
        }
    }
}

CustomizeMenuDialog::~CustomizeMenuDialog()
{
    delete ui;
}
