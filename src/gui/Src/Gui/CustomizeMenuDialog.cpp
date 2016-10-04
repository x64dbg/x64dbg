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
        switch(i.first->getId())
        {
        case 1://CPUDisassembly
            viewName = tr("Disassembler");
            break;
        case 2:
            viewName = tr("Dump");
            break;
        case 7:
            viewName = tr("Watch");
            break;
        case 17:
            viewName = tr("Call Staack");
            break;
        case 25:
            viewName = tr("Threads");
            break;
        /*
        3:CPUStack
        4:Registers
        5:Info box
        6:Arguments
        8:Graph
        9:Log
        10:Notes
        11:Breakpoints-Software
        12:Breakpoints-Hardware
        13:Breakpoints-Memory
        14:Breakpoints-Exception
        15:Breakpoints-DLL
        16:MemoryMap
        18:Callstack
        19:SEH
        20:Script
        21:Symbols-Modules
        22:Symbols-Symbols
        23:Source
        24:References
        26:Handles-Handles
        27:Handles-TCP Connections
        28:Handles-Privileges
         */
        default:
            break;
        }
        if(viewName.isEmpty())
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

CustomizeMenuDialog::~CustomizeMenuDialog()
{
    delete ui;
}
