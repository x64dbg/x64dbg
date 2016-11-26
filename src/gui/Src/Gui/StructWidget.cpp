#include "StructWidget.h"
#include "ui_StructWidget.h"

struct TypeDescriptor
{
    TYPEDESCRIPTOR type;
    QString name;
};
Q_DECLARE_METATYPE(TypeDescriptor)

StructWidget::StructWidget(QWidget* parent) :
    QWidget(parent),
    ui(new Ui::StructWidget)
{
    ui->setupUi(this);
    ui->treeWidget->setStyleSheet("QTreeWidget { color: #000000; background-color: #FFF8F0; alternate-background-color: #DCD9CF; }");
    connect(Bridge::getBridge(), SIGNAL(typeAddNode(void*, const TYPEDESCRIPTOR*)), this, SLOT(typeAddNode(void*, const TYPEDESCRIPTOR*)));
    connect(Bridge::getBridge(), SIGNAL(typeClear()), this, SLOT(typeClear()));
    connect(Bridge::getBridge(), SIGNAL(typeUpdateWidget()), this, SLOT(typeUpdateWidget()));
    setupColumns();
}

StructWidget::~StructWidget()
{
    delete ui;
}

void StructWidget::typeAddNode(void* parent, const TYPEDESCRIPTOR* type)
{
    TypeDescriptor dtype;
    dtype.type = *type;
    dtype.name = QString(dtype.type.name);
    dtype.type.name = nullptr;
    auto text = QStringList() << dtype.name << ToPtrString(dtype.type.addr) << "0x" + ToHexString(dtype.type.size);
    QTreeWidgetItem* item = parent ? new QTreeWidgetItem((QTreeWidgetItem*)parent, text) : new QTreeWidgetItem(ui->treeWidget, text);
    item->setExpanded(dtype.type.expanded);
    QVariant var;
    var.setValue(dtype);
    item->setData(0, Qt::UserRole, var);
    Bridge::getBridge()->setResult(dsint(item));
}

void StructWidget::typeClear()
{
    ui->treeWidget->clear();
    Bridge::getBridge()->setResult();
}

void StructWidget::typeUpdateWidget()
{
    QTreeWidgetItemIterator it(ui->treeWidget);
    while(*it)
    {
        QString valueStr;
        QTreeWidgetItem* item = *it;
        auto type = item->data(0, Qt::UserRole).value<TypeDescriptor>();
        if(type.type.callback) //use the provided callback
        {
            char value[128] = "";
            size_t valueCount = _countof(value);
            if(!type.type.callback(&type.type, value, &valueCount) && valueCount && valueCount != _countof(value))
            {
                auto dest = new char[valueCount];
                if(type.type.callback(&type.type, dest, &valueCount))
                    valueStr = value;
                else
                    valueStr = "???";
                delete[] dest;
            }
            else
                valueStr = value;
        }
        else if(!item->childCount() && type.type.size > 0 && type.type.size <= sizeof(uint64_t)) //attempt to display small, non-parent values
        {
            uint64_t data;
            if(DbgMemRead(type.type.addr, &data, type.type.size))
                valueStr = QString().sprintf("0x%llX, %llu", data, data, data);
            else if(type.type.addr)
                valueStr = "???";
        }
        item->setText(3, valueStr);
        ++it;
    }
}

void StructWidget::setupColumns()
{
    ui->treeWidget->setColumnWidth(0, 200); //Name
    ui->treeWidget->setColumnWidth(1, 80); //Address
    ui->treeWidget->setColumnWidth(2, 80); //Size
    //ui->treeWidget->setColumnWidth(3, 80); //Value
}
