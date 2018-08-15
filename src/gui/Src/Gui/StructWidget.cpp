#include "StructWidget.h"
#include "ui_StructWidget.h"
#include "Configuration.h"
#include "MenuBuilder.h"
#include "LineEditDialog.h"
#include "GotoDialog.h"
#include <QFileDialog>
#include "StringUtil.h"
#include "MiscUtil.h"

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
    connect(Bridge::getBridge(), SIGNAL(dbgStateChanged(DBGSTATE)), this, SLOT(dbgStateChangedSlot(DBGSTATE)));
    connect(Config(), SIGNAL(colorsUpdated()), this, SLOT(colorsUpdatedSlot()));
    connect(Config(), SIGNAL(fontsUpdated()), this, SLOT(fontsUpdatedSlot()));
    connect(Config(), SIGNAL(shortcutsUpdated()), this, SLOT(shortcutsUpdatedSlot()));
    colorsUpdatedSlot();
    fontsUpdatedSlot();
    setupContextMenu();
    setupColumns();
}

StructWidget::~StructWidget()
{
    delete ui;
}

void StructWidget::colorsUpdatedSlot()
{
    auto color = ConfigColor("AbstractTableViewTextColor");
    auto background = ConfigColor("StructBackgroundColor");
    auto altBackground = ConfigColor("StructAlternateBackgroundColor");
    auto style = QString("QTreeWidget { color: %1; background-color: %2; alternate-background-color: %3; }").arg(color.name(), background.name(), altBackground.name());
    ui->treeWidget->setStyleSheet(style);
}

void StructWidget::fontsUpdatedSlot()
{
    auto font = ConfigFont("AbstractTableView");
    setFont(font);
    ui->treeWidget->setFont(font);
    ui->treeWidget->header()->setFont(font);
}

void StructWidget::shortcutsUpdatedSlot()
{
    updateShortcuts();
}

void StructWidget::typeAddNode(void* parent, const TYPEDESCRIPTOR* type)
{
    TypeDescriptor dtype;
    dtype.type = *type;
    dtype.name = QString(dtype.type.name);
    dtype.type.name = nullptr;
    auto text = QStringList() << dtype.name << ToPtrString(dtype.type.addr + dtype.type.offset) << "0x" + ToHexString(dtype.type.size);
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
    for(QTreeWidgetItemIterator it(ui->treeWidget); *it; ++it)
    {
        QTreeWidgetItem* item = *it;
        auto type = item->data(0, Qt::UserRole).value<TypeDescriptor>();
        auto name = type.name.toUtf8();
        type.type.name = name.constData();
        auto addr = type.type.addr + type.type.offset;
        item->setText(1, ToPtrString(addr));
        QString valueStr;
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
            if(DbgMemRead(addr, &data, type.type.size))
            {
                if(type.type.reverse)
                    std::reverse((char*)data, (char*)data + type.type.size);
                valueStr = QString().sprintf("0x%llX, %llu", data, data, data);
            }
            else if(type.type.addr)
                valueStr = "???";
        }
        item->setText(3, valueStr);
    }
}

void StructWidget::dbgStateChangedSlot(DBGSTATE state)
{
    if(state == stopped)
        typeClear();
}

void StructWidget::setupColumns()
{
    ui->treeWidget->setColumnWidth(0, 300); //Name
    ui->treeWidget->setColumnWidth(1, 80); //Address
    ui->treeWidget->setColumnWidth(2, 80); //Size
    //ui->treeWidget->setColumnWidth(3, 80); //Value
}

#define hasSelection !!ui->treeWidget->selectedItems().count()
#define selectedItem ui->treeWidget->selectedItems()[0]
#define selectedType selectedItem->data(0, Qt::UserRole).value<TypeDescriptor>().type

void StructWidget::setupContextMenu()
{
    mMenuBuilder = new MenuBuilder(this);
    mMenuBuilder->addAction(makeAction(DIcon("dump.png"), tr("&Follow in Dump"), SLOT(followDumpSlot())), [this](QMenu*)
    {
        return hasSelection && DbgMemIsValidReadPtr(selectedType.addr + selectedType.offset);
    });
    mMenuBuilder->addAction(makeAction(DIcon("structaddr.png"), tr("Change address"), SLOT(changeAddrSlot())), [this](QMenu*)
    {
        return hasSelection && !selectedItem->parent() && DbgIsDebugging();
    });
    mMenuBuilder->addAction(makeAction(DIcon("visitstruct.png"), tr("Visit type"), SLOT(visitSlot())));
    mMenuBuilder->addAction(makeAction(DIcon("database-import.png"), tr("Load JSON"), SLOT(loadJsonSlot())));
    mMenuBuilder->addAction(makeAction(DIcon("source.png"), tr("Parse header"), SLOT(parseFileSlot())));
    mMenuBuilder->addAction(makeAction(DIcon("removestruct.png"), tr("Remove"), SLOT(removeSlot())), [this](QMenu*)
    {
        return hasSelection && !selectedItem->parent();
    });
    mMenuBuilder->addAction(makeAction(DIcon("eraser.png"), tr("Clear"), SLOT(clearSlot())));
    mMenuBuilder->addAction(makeShortcutAction(DIcon("sync.png"), tr("&Refresh"), SLOT(refreshSlot()), "ActionRefresh"));
    mMenuBuilder->loadFromConfig();
}

void StructWidget::on_treeWidget_customContextMenuRequested(const QPoint & pos)
{
    QMenu wMenu;
    mMenuBuilder->build(&wMenu);
    if(wMenu.actions().count())
        wMenu.exec(ui->treeWidget->viewport()->mapToGlobal(pos));
}

void StructWidget::followDumpSlot()
{
    if(!hasSelection)
        return;
    DbgCmdExec(QString("dump %1").arg(ToPtrString(selectedType.addr + selectedType.offset)).toUtf8().constData());
}

void StructWidget::clearSlot()
{
    ui->treeWidget->clear();
}

void StructWidget::removeSlot()
{
    if(!hasSelection)
        return;
    delete selectedItem;
}

void StructWidget::visitSlot()
{
    //TODO: replace with a list to pick from
    LineEditDialog mLineEdit(this);
    mLineEdit.setWindowTitle(tr("Type to visit"));
    if(mLineEdit.exec() != QDialog::Accepted || !mLineEdit.editText.length())
        return;
    if(!mGotoDialog)
        mGotoDialog = new GotoDialog(this);
    duint addr = 0;
    mGotoDialog->setWindowTitle(tr("Address to visit"));
    if(DbgIsDebugging() && mGotoDialog->exec() == QDialog::Accepted)
        addr = DbgValFromString(mGotoDialog->expressionText.toUtf8().constData());
    DbgCmdExec(QString("VisitType %1, %2").arg(mLineEdit.editText, ToPtrString(addr)).toUtf8().constData());
}

void StructWidget::loadJsonSlot()
{
    auto filename = QFileDialog::getOpenFileName(this, tr("Load JSON"), QString(), tr("JSON files (*.json);;All files (*.*)"));
    if(!filename.length())
        return;
    filename = QDir::toNativeSeparators(filename);
    DbgCmdExec(QString("LoadTypes \"%1\"").arg(filename).toUtf8().constData());
}

void StructWidget::parseFileSlot()
{
    auto filename = QFileDialog::getOpenFileName(this, tr("Parse header"), QString(), tr("Header files (*.h *.hpp);;All files (*.*)"));
    if(!filename.length())
        return;
    filename = QDir::toNativeSeparators(filename);
    DbgCmdExec(QString("ParseTypes \"%1\"").arg(filename).toUtf8().constData());
}

static void changeTypeAddr(QTreeWidgetItem* item, duint addr)
{
    auto changeAddr = item->data(0, Qt::UserRole).value<TypeDescriptor>().type.addr;
    for(QTreeWidgetItemIterator it(item); *it; ++it)
    {
        QTreeWidgetItem* item = *it;
        auto type = item->data(0, Qt::UserRole).value<TypeDescriptor>();
        type.type.addr = type.type.addr == changeAddr ? addr : 0; //invalidate pointers (requires revisit)
        QVariant var;
        var.setValue(type);
        item->setData(0, Qt::UserRole, var);
    }
}

void StructWidget::changeAddrSlot()
{
    if(!hasSelection || !DbgIsDebugging())
        return;
    if(!mGotoDialog)
        mGotoDialog = new GotoDialog(this);
    mGotoDialog->setWindowTitle(tr("Change address"));
    if(mGotoDialog->exec() != QDialog::Accepted)
        return;
    changeTypeAddr(selectedItem, DbgValFromString(mGotoDialog->expressionText.toUtf8().constData()));
    refreshSlot();
}

void StructWidget::refreshSlot()
{
    typeUpdateWidget();
}
