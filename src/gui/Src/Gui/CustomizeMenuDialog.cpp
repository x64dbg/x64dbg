#include "CustomizeMenuDialog.h"
#include "ui_CustomizeMenuDialog.h"
#include "MenuBuilder.h"
#include "Configuration.h"

CustomizeMenuDialog::CustomizeMenuDialog(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::CustomizeMenuDialog)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint | Qt::MSWindowsFixedSizeDialogHint);
    ui->setupUi(this);
    for(const Configuration::MenuMap & i : Config()->NamedMenuBuilders)
    {
        QString viewName;
        MenuBuilder* builder = nullptr;
        QList<QAction*>* mainMenuList = nullptr;
        QString id;
        if(i.type == 1)
        {
            mainMenuList = i.mainMenuList;
            id = mainMenuList->first()->text();
        }
        else if(i.type == 0)
        {
            builder = i.builder;
            id = builder->getId();
        }
        else //invalid or unsupported type.Continue
            continue;
        //Get localized string for the name of individual views
        if(id == "CPUDisassembly")
            viewName = tr("Disassembler");
        else if(id == "CPUDump")
            viewName = tr("Dump");
        else if(id == "WatchView")
            viewName = tr("Watch");
        else if(id == "CallStackView")
            viewName = tr("Call Stack");
        else if(id == "ThreadView")
            viewName = tr("Threads");
        else if(id == "DisassemblerGraphView")
            viewName = tr("Graph");
        else if(id == "XrefBrowseDialog")
            viewName = tr("Xref Browser");
        else if(id == "StructWidget")
            viewName = tr("Struct");
        else if(id == "CPUStack")
            viewName = tr("Stack");
        else if(id == "SourceView")
            viewName = tr("Source");
        else if(id == "File")
            viewName = tr("File");
        else if(id == "Debug")
            viewName = tr("Debug");
        else if(id == "Option")
            viewName = tr("Option");
        else if(id == "Favourite")
            viewName = tr("Favourite");
        else if(id == "Help")
            viewName = tr("Help");
        else if(id == "View")
            viewName = tr("View");
        else if(id == "TraceBrowser")
            viewName = tr("Trace disassembler");
        else if(id == "TraceDump")
            viewName = tr("Trace dump");
        else
            continue;
        // Add Parent Node
        QTreeWidgetItem* parentItem = new QTreeWidgetItem(ui->treeWidget);
        parentItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        parentItem->setText(0, viewName);
        // Add Children nodes
        for(size_t j = 0; j < i.count; j++)
        {
            // Get localized name of menu item by communicating with the menu
            QString text;
            if(i.type == 0)
                text = builder->getText(j);
            else if(i.type == 1)
                text = mainMenuList->at(int(j + 1))->text();
            // Add a child node only if it has a non-empty name
            if(!text.isEmpty())
            {
                QTreeWidgetItem* menuItem = new QTreeWidgetItem(parentItem, 0);
                menuItem->setText(0, text.replace(QChar('&'), ""));
                QString configString = QString("Menu%1Hidden%2").arg(id).arg(j);
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
