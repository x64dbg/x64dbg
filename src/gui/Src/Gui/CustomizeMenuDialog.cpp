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
    for(const auto & i : Config()->NamedMenuBuilders)
    {
        QString viewName;
        MenuBuilder* builder = nullptr;
        QList<QString>* mainMenuList = nullptr;
        const char* id;
        if(std::get<1>(i) == 1)
        {
            mainMenuList = reinterpret_cast<QList<QString>*>(std::get<0>(i));
            id = _strdup(mainMenuList->at(0).toUtf8().constData()); // must use string duplication here because constData may be a temporary data storage.
        }
        else if(std::get<1>(i) == 0)
        {
            builder = reinterpret_cast<MenuBuilder*>(std::get<0>(i));
            id = builder->getId();
        }
        else
            continue;
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
        else if(strcmp(id, "DisassemblerGraphView") == 0)
            viewName = tr("Graph");
        else if(strcmp(id, "CPUStack") == 0)
            viewName = tr("Stack");
        else if(strcmp(id, "File") == 0)
            viewName = tr("File");
        else if(strcmp(id, "Debug") == 0)
            viewName = tr("Debug");
        else if(strcmp(id, "Plugin") == 0)
            viewName = tr("Plugin");
        else if(strcmp(id, "Option") == 0)
            viewName = tr("Option");
        else if(strcmp(id, "Favourite") == 0)
            viewName = tr("Favourite");
        else if(strcmp(id, "Help") == 0)
            viewName = tr("Help");
        else if(strcmp(id, "View") == 0)
            viewName = tr("View");
        else
        {
            if(std::get<1>(i) == 1)
                free((char*)id);
            id = nullptr;
            continue;
        }
        QTreeWidgetItem* parentItem = new QTreeWidgetItem(ui->treeWidget);
        parentItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        parentItem->setText(0, viewName);
        for(size_t j = 0; j < std::get<2>(i); j++)
        {
            QString text;
            if(std::get<1>(i) == 0)
                text = builder->getText(j);
            else if(std::get<1>(i) == 1)
                text = mainMenuList->at(j + 1);
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
        if(std::get<1>(i) == 1)
            free((char*)id);
        id = nullptr;
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
