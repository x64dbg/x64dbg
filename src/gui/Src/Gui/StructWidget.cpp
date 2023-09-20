#include "StructWidget.h"
#include "ui_StructWidget.h"
#include "Configuration.h"
#include "MenuBuilder.h"
#include "LineEditDialog.h"
#include "GotoDialog.h"
#include <QFileDialog>
#include "StringUtil.h"
#include "MiscUtil.h"
#include "RichTextItemDelegate.h"

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
    ui->treeWidget->setStyleSheet("QTreeWidget { background-color: #FFF8F0; alternate-background-color: #DCD9CF; }");
    ui->treeWidget->setItemDelegate(new RichTextItemDelegate(&mTextColor, ui->treeWidget));
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

void StructWidget::saveWindowSettings()
{
    auto saveColumn = [this](int column)
    {
        auto settingName = QString("StructWidgetColumn%1").arg(column);
        BridgeSettingSetUint("Gui", settingName.toUtf8().constData(), ui->treeWidget->columnWidth(column));
    };
    saveColumn(0);
    saveColumn(1);
    saveColumn(2);
    saveColumn(3);
}

void StructWidget::loadWindowSettings()
{
    auto loadColumn = [this](int column)
    {
        auto settingName = QString("StructWidgetColumn%1").arg(column);
        duint width = 0;
        if(BridgeSettingGetUint("Gui", settingName.toUtf8().constData(), &width))
            ui->treeWidget->setColumnWidth(column, width);
    };
    loadColumn(0);
    loadColumn(1);
    loadColumn(2);
    loadColumn(3);
}

void StructWidget::colorsUpdatedSlot()
{
    mTextColor = ConfigColor("StructTextColor");
    auto background = ConfigColor("StructBackgroundColor");
    auto altBackground = ConfigColor("StructAlternateBackgroundColor");
    auto style = QString("QTreeWidget { background-color: %1; alternate-background-color: %2; }").arg(background.name(), altBackground.name());
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
    // Disable updates until the next typeUpdateWidget()
    ui->treeWidget->setUpdatesEnabled(false);

    TypeDescriptor dtype;
    dtype.type = *type;
    dtype.name = highlightTypeName(dtype.type.name);
    dtype.type.name = nullptr;
    auto text = QStringList() << dtype.name << ToPtrString(dtype.type.addr + dtype.type.offset) << "0x" + ToHexString(dtype.type.size);
    QTreeWidgetItem* item = parent ? new QTreeWidgetItem((QTreeWidgetItem*)parent, text) : new QTreeWidgetItem(ui->treeWidget, text);
    item->setExpanded(dtype.type.expanded);
    QVariant var;
    var.setValue(dtype);
    item->setData(0, Qt::UserRole, var);
    Bridge::getBridge()->setResult(BridgeResult::TypeAddNode, dsint(item));
}

void StructWidget::typeClear()
{
    ui->treeWidget->clear();
    Bridge::getBridge()->setResult(BridgeResult::TypeClear);
}

void StructWidget::typeUpdateWidget()
{
    ui->treeWidget->setUpdatesEnabled(false);
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
    ui->treeWidget->setUpdatesEnabled(true);
}

void StructWidget::dbgStateChangedSlot(DBGSTATE state)
{
    if(state == stopped)
        ui->treeWidget->clear();
}

void StructWidget::setupColumns()
{
    auto charWidth = ui->treeWidget->fontMetrics().width(' ');
    ui->treeWidget->setColumnWidth(0, 4 + charWidth * 60); //Name
    ui->treeWidget->setColumnWidth(1, 6 + charWidth * sizeof(duint) * 2); //Address
    ui->treeWidget->setColumnWidth(2, 4 + charWidth * 6); //Size
}

#define hasSelection !!ui->treeWidget->selectedItems().count()
#define selectedItem ui->treeWidget->selectedItems()[0]
#define selectedType selectedItem->data(0, Qt::UserRole).value<TypeDescriptor>().type

void StructWidget::setupContextMenu()
{
    mMenuBuilder = new MenuBuilder(this);
    mMenuBuilder->addAction(makeAction(DIcon("dump"), tr("Follow value in Dump"), SLOT(followValueDumpSlot())), [this](QMenu*)
    {
        return DbgMemIsValidReadPtr(selectedValue());
    });
    mMenuBuilder->addAction(makeAction(DIcon("processor-cpu"), tr("Follow value in Disassembler"), SLOT(followValueDisasmSlot())), [this](QMenu*)
    {
        return DbgMemIsValidReadPtr(selectedValue());
    });
    mMenuBuilder->addAction(makeAction(DIcon("dump"), tr("&Follow address in Dump"), SLOT(followDumpSlot())), [this](QMenu*)
    {
        return hasSelection && DbgMemIsValidReadPtr(selectedType.addr + selectedType.offset);
    });
    mMenuBuilder->addAction(makeAction(DIcon("structaddr"), tr("Change address"), SLOT(changeAddrSlot())), [this](QMenu*)
    {
        return hasSelection && !selectedItem->parent() && DbgIsDebugging();
    });
    mMenuBuilder->addAction(makeAction(DIcon("visitstruct"), tr("Visit type"), SLOT(visitSlot())));
    mMenuBuilder->addAction(makeAction(DIcon("database-import"), tr("Load JSON"), SLOT(loadJsonSlot())));
    mMenuBuilder->addAction(makeAction(DIcon("source"), tr("Parse header"), SLOT(parseFileSlot())));
    mMenuBuilder->addAction(makeAction(DIcon("removestruct"), tr("Remove"), SLOT(removeSlot())), [this](QMenu*)
    {
        return hasSelection && !selectedItem->parent();
    });
    mMenuBuilder->addAction(makeAction(DIcon("eraser"), tr("Clear"), SLOT(clearSlot())));
    mMenuBuilder->addAction(makeShortcutAction(DIcon("sync"), tr("&Refresh"), SLOT(refreshSlot()), "ActionRefresh"));
    mMenuBuilder->loadFromConfig();
}

QString StructWidget::highlightTypeName(QString name) const
{
    // TODO: this can be improved with colors
    static auto re = []
    {
        const char* keywords[] =
        {
            "uint64_t",
            "uint32_t",
            "uint16_t",
            "char16_t",
            "unsigned",
            "int64_t",
            "int32_t",
            "wchar_t",
            "int16_t",
            "uint8_t",
            "struct",
            "double",
            "size_t",
            "uint64",
            "uint32",
            "ushort",
            "uint16",
            "signed",
            "int8_t",
            "union",
            "const",
            "float",
            "duint",
            "dsint",
            "int64",
            "int32",
            "short",
            "int16",
            "ubyte",
            "uchar",
            "uint8",
            "void",
            "long",
            "bool",
            "byte",
            "char",
            "int8",
            "ptr",
            "int",
        };
        QString keywordRegex;
        keywordRegex += "\\b(";
        for(size_t i = 0; i < _countof(keywords); i++)
        {
            if(i > 0)
                keywordRegex += '|';
            keywordRegex += QRegExp::escape(keywords[i]);
        }
        keywordRegex += ")\\b";
        return QRegExp(keywordRegex, Qt::CaseSensitive);
    }();

    name.replace(re, "<b>\\1</b>");
    return std::move(name);
}

duint StructWidget::selectedValue() const
{
    if(!hasSelection)
        return 0;
    QStringList split = selectedItem->text(3).split(',');
    if(split.length() < 1)
        return 0;
    return split[0].toULongLong(nullptr, 0);
}

void StructWidget::on_treeWidget_customContextMenuRequested(const QPoint & pos)
{
    QMenu menu;
    mMenuBuilder->build(&menu);
    if(menu.actions().count())
        menu.exec(ui->treeWidget->viewport()->mapToGlobal(pos));
}

void StructWidget::followDumpSlot()
{
    if(!hasSelection)
        return;
    DbgCmdExec(QString("dump %1").arg(ToPtrString(selectedType.addr + selectedType.offset)));
}

void StructWidget::followValueDumpSlot()
{
    if(!hasSelection)
        return;
    DbgCmdExec(QString("dump %1").arg(ToPtrString(selectedValue())));
}

void StructWidget::followValueDisasmSlot()
{
    if(!hasSelection)
        return;
    DbgCmdExec(QString("disasm %1").arg(ToPtrString(selectedValue())));
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
    DbgCmdExec(QString("VisitType %1, %2, 2").arg(mLineEdit.editText, ToPtrString(addr)));
}

void StructWidget::loadJsonSlot()
{
    auto filename = QFileDialog::getOpenFileName(this, tr("Load JSON"), QString(), tr("JSON files (*.json);;All files (*.*)"));
    if(!filename.length())
        return;
    filename = QDir::toNativeSeparators(filename);
    DbgCmdExec(QString("LoadTypes \"%1\"").arg(filename));
}

void StructWidget::parseFileSlot()
{
    auto filename = QFileDialog::getOpenFileName(this, tr("Parse header"), QString(), tr("Header files (*.h *.hpp);;All files (*.*)"));
    if(!filename.length())
        return;
    filename = QDir::toNativeSeparators(filename);
    DbgCmdExec(QString("ParseTypes \"%1\"").arg(filename));
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
