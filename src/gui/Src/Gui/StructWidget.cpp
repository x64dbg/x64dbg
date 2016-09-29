#include "StructWidget.h"
#include "ui_StructWidget.h"

StructWidget::StructWidget(QWidget* parent) :
    QWidget(parent),
    ui(new Ui::StructWidget)
{
    ui->setupUi(this);
    ui->treeWidget->setStyleSheet("QTreeWidget { color: #000000; background-color: #FFF8F0; alternate-background-color: #DCD9CF; }");
    showTree();
}

StructWidget::~StructWidget()
{
    delete ui;
}

void StructWidget::showTree()
{
    ui->treeWidget->clear();
    QTreeWidgetItem* t = new QTreeWidgetItem(ui->treeWidget, QStringList() << "struct TEST");
    new QTreeWidgetItem(t, QStringList() << "int a" << "00000000" << "0xA");
    new QTreeWidgetItem(t, QStringList() << "char b" << "00000004" << "0xB");
    QTreeWidgetItem* e = new QTreeWidgetItem(t, QStringList() << "struct BLUB");
    new QTreeWidgetItem(e, QStringList() << "short c" << "00000005" << "0xC");
    QTreeWidgetItem* d = new QTreeWidgetItem(e, QStringList() << "int[2]");
    new QTreeWidgetItem(d, QStringList() << "int d[0]" << "00000007" << "0xD0");
    new QTreeWidgetItem(d, QStringList() << "int d[1]" << "00000011" << "0xD1");
    new QTreeWidgetItem(t, QStringList() << "int f" << "00000015" << "0xF");
    ui->treeWidget->setColumnWidth(0, 200);
    ui->treeWidget->setColumnWidth(1, 80);
}
