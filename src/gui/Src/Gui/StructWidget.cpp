#include <QFileDialog>
#include <QTextDocumentFragment>

#include "Configuration.h"
#include "GotoDialog.h"
#include "MenuBuilder.h"
#include "MiscUtil.h"
#include "RichTextItemDelegate.h"
#include "StringUtil.h"
#include "StructWidget.h"
#include "ui_StructWidget.h"

struct TypeDescriptor
{
    TYPEDESCRIPTOR type = {};
    QString name;
    QString typeName;
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
    setupColumns();
    setupContextMenu();
}

StructWidget::~StructWidget()
{
    delete ui;
}

void StructWidget::saveWindowSettings()
{
    auto saveColumn = [this](int column)
    {
        auto settingName = QString("StructWidgetV2Column%1").arg(column);
        BridgeSettingSetUint("Gui", settingName.toUtf8().constData(), ui->treeWidget->columnWidth(column));
    };
    auto columnCount = ui->treeWidget->columnCount();
    for(int i = 0; i < columnCount; i++)
        saveColumn(i);
}

void StructWidget::loadWindowSettings()
{
    auto loadColumn = [this](int column)
    {
        auto settingName = QString("StructWidgetV2Column%1").arg(column);
        duint width = 0;
        if(BridgeSettingGetUint("Gui", settingName.toUtf8().constData(), &width))
            ui->treeWidget->setColumnWidth(column, width);
    };
    auto columnCount = ui->treeWidget->columnCount();
    for(int i = 0; i < columnCount; i++)
        loadColumn(i);
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
    if(type->magic == TYPEDESCRIPTOR_MAGIC)
    {
        dtype.type = *type;
    }
    else
    {
        // NOTE: This path is for binary backwards compatibility with the following structure:
        // https://github.com/x64dbg/x64dbg/blob/1d3aa9b40716b42d086a90619db01fa12532eea6/src/bridge/bridgemain.h#L1339-L1350
        dtype.type.expanded = type->expanded;
        dtype.type.reverse = type->reverse;
        dtype.type.magic = TYPEDESCRIPTOR_MAGIC;
        dtype.type.name = type->name;
        dtype.type.addr = type->addr;
        dtype.type.offset = type->offset;
        dtype.type.id = type->id;
        dtype.type.sizeBits = type->sizeBits * 8;
        dtype.type.callback = type->callback;
        dtype.type.userdata = type->userdata;
        dtype.type.typeName = nullptr;
        dtype.type.bitOffset = 0;
    }

    dtype.name = highlightTypeName(dtype.type.name);
    if(dtype.type.typeName != nullptr)
    {
        dtype.typeName = QString(dtype.type.typeName);
    }

    // NOTE: The lifetime for these is expected to end until we return here, which is
    // why we set these to nullptr (use the QString in dtype instead)
    dtype.type.name = nullptr;
    dtype.type.typeName = nullptr;

    QStringList text;
    auto columnCount = ui->treeWidget->columnCount();
    for(int i = 0; i < columnCount; i++)
        text.append(QString());

    text[ColOffset] = "+0x" + ToHexString(dtype.type.offset);
    text[ColField] = dtype.name;
    if(dtype.type.offset == 0)
        text[ColAddress] = QString("<u>%1</u>").arg(ToPtrString(dtype.type.addr + dtype.type.offset));
    else
        text[ColAddress] = ToPtrString(dtype.type.addr + dtype.type.offset);
    text[ColSize] = "0x" + ToHexString(dtype.type.sizeBits / 8);
    text[ColValue] = ""; // NOTE: filled in later

    QTreeWidgetItem* item = nullptr;
    if(parent == nullptr)
    {
        if(mInsertIndex != -1)
        {
            item = new QTreeWidgetItem(text);
            ui->treeWidget->insertTopLevelItem(mInsertIndex, item);

            mInsertIndex = -1;
        }
        else
        {
            item = new QTreeWidgetItem(ui->treeWidget, text);
        }

        // Scroll to the new root node in typeUpdateWidget
        mScrollItem = dtype.type.expanded ? item : nullptr;
    }
    else
    {
        item = new QTreeWidgetItem((QTreeWidgetItem*)parent, text);
    }

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
        auto typeName = type.typeName.toUtf8();
        type.type.name = name.constData();
        type.type.typeName = typeName.constData();

        auto addr = type.type.addr + type.type.offset;
        item->setText(ColAddress, ToPtrString(addr));
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
        else if(!item->childCount() && type.type.sizeBits > 0 && type.type.sizeBits / 8 <= sizeof(uint64_t)) //attempt to display small, non-parent values
        {
            uint64_t data;
            // todo fix mem read for bit offset
            if(DbgMemRead(addr, &data, type.type.sizeBits / 8))
            {
                if(type.type.reverse)
                    std::reverse((char*)data, (char*)data + (type.type.sizeBits / 8));
                valueStr = QString().sprintf("0x%llX, %llu", data, data, data);
            }
            else if(type.type.addr)
                valueStr = "???";
        }

        item->setText(ColValue, valueStr);
    }
    ui->treeWidget->setUpdatesEnabled(true);

    // Scroll to the newly-created node (typeAddNode)
    if(mScrollItem != nullptr)
    {
        ui->treeWidget->scrollToItem(mScrollItem, QAbstractItemView::PositionAtTop);
        ui->treeWidget->setCurrentItem(mScrollItem);
        mScrollItem = nullptr;
    }
}

void StructWidget::dbgStateChangedSlot(DBGSTATE state)
{
    if(state == stopped)
        ui->treeWidget->clear();
}

void StructWidget::setupColumns()
{
    auto charWidth = ui->treeWidget->fontMetrics().width(' ');
    ui->treeWidget->setColumnWidth(ColField, 4 + charWidth * 60);
    ui->treeWidget->setColumnWidth(ColOffset, 6 + charWidth * 7);
    ui->treeWidget->setColumnWidth(ColAddress, 6 + charWidth * sizeof(duint) * 2);
    ui->treeWidget->setColumnWidth(ColSize, 4 + charWidth * 6);

    // NOTE: Trick to display the expander icons in the second column
    // Reference: https://stackoverflow.com/a/25887454/1806760
    // ui->treeWidget->header()->moveSection(ColField, ColOffset);
}

#define hasSelection !!ui->treeWidget->selectedItems().count()
#define selectedItem ui->treeWidget->selectedItems()[0]
#define selectedType selectedItem->data(0, Qt::UserRole).value<TypeDescriptor>().type

void StructWidget::setupContextMenu()
{
    mMenuBuilder = new MenuBuilder(this);
    mMenuBuilder->addAction(makeAction(DIcon("dump"), tr("&Follow address in Dump"), SLOT(followDumpSlot())), [this](QMenu*)
    {
        return hasSelection && DbgMemIsValidReadPtr(selectedType.addr + selectedType.offset);
    });
    mMenuBuilder->addAction(makeAction(DIcon("dump"), tr("Follow value in Dump"), SLOT(followValueDumpSlot())), [this](QMenu*)
    {
        return DbgMemIsValidReadPtr(selectedValue());
    });
    mMenuBuilder->addAction(makeAction(DIcon("processor-cpu"), tr("Follow value in Disassembler"), SLOT(followValueDisasmSlot())), [this](QMenu*)
    {
        return DbgMemIsValidReadPtr(selectedValue());
    });
    mMenuBuilder->addAction(makeDescAction(DIcon("structaddr"), tr("Reload type"), tr("Reload the type from the database and display it (at a different address)."), SLOT(reloadTypeSlot())), [this](QMenu*)
    {
        return hasSelection && !selectedItem->parent() && DbgIsDebugging();
    });
    mMenuBuilder->addAction(makeAction(DIcon("visitstruct"), tr("Display type"), SLOT(displayTypeSlot())));
    mMenuBuilder->addAction(makeAction(DIcon("database-import"), tr("Load JSON"), SLOT(loadJsonSlot())));
    mMenuBuilder->addAction(makeAction(DIcon("source"), tr("Parse header"), SLOT(parseFileSlot())));
    mMenuBuilder->addAction(makeAction(DIcon("removestruct"), tr("Remove"), SLOT(removeSlot())), [this](QMenu*)
    {
        return hasSelection && !selectedItem->parent();
    });
    mMenuBuilder->addAction(makeAction(DIcon("eraser"), tr("Clear displayed types"), SLOT(clearSlot())));
    mMenuBuilder->addAction(makeShortcutDescAction(DIcon("sync"), tr("&Refresh values"), tr("Quickly refresh the values, without reloading the type."), SLOT(typeUpdateWidget()), "ActionRefresh"));

    auto copyMenu = new MenuBuilder(this);
    auto columnCount = ui->treeWidget->columnCount();
    auto headerItem = ui->treeWidget->headerItem();
    for(int column = 0; column < columnCount; column++)
    {
        auto action = makeAction(headerItem->text(column), SLOT(copyColumnSlot()));
        action->setObjectName(QString("%1").arg(column));
        copyMenu->addAction(action, [this, column](QMenu*)
        {
            return hasSelection && !selectedItem->text(column).isEmpty();
        });
    }
    mMenuBuilder->addMenu(makeMenu(DIcon("copy"), tr("&Copy")), copyMenu);

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
            "double",
            "size_t",
            "uint64",
            "uint32",
            "ushort",
            "uint16",
            "signed",
            "int8_t",
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

    static QRegExp sre("^(struct|union|class|enum) ([a-zA-Z0-9_:$]+)");
    name.replace(sre, "<u>\\1</u> <b>\\2</b>");

    return name;
}

duint StructWidget::selectedValue() const
{
    if(!hasSelection)
        return 0;
    QStringList split = selectedItem->text(ColValue).split(',');
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

void StructWidget::displayTypeSlot()
{
    QStringList structs;
    DbgFunctions()->EnumStructs([](const char* name, void* userdata)
    {
        ((QStringList*)userdata)->append(name);
    }, &structs);
    if(structs.isEmpty())
    {
        SimpleErrorBox(this, tr("Error"), tr("No types loaded yet, parse a header first..."));
        return;
    }

    QString selection;
    if(!SimpleChoiceBox(this, tr("Type to display"), "", structs, selection, true, "", DIcon("struct"), 1) || selection.isEmpty())
        return;
    if(!mGotoDialog)
        mGotoDialog = new GotoDialog(this);
    duint addr = 0;
    mGotoDialog->setWindowTitle(tr("Address to display %1 at").arg(selection));
    if(DbgIsDebugging() && mGotoDialog->exec() == QDialog::Accepted)
        addr = DbgValFromString(mGotoDialog->expressionText.toUtf8().constData());
    DbgCmdExec(QString("DisplayType %1, %2").arg(selection, ToPtrString(addr)));
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

void StructWidget::reloadTypeSlot()
{
    if(!hasSelection || selectedItem->parent() || !DbgIsDebugging())
        return;

    auto type = selectedItem->data(0, Qt::UserRole).value<TypeDescriptor>();

    if(!mGotoDialog)
        mGotoDialog = new GotoDialog(this);
    mGotoDialog->setInitialExpression(ToPtrString(selectedType.addr));
    mGotoDialog->setWindowTitle(tr("Address to display %1 at").arg(type.typeName));
    if(mGotoDialog->exec() != QDialog::Accepted)
        return;

    // The callbacks invoked by DisplayType will insert the type at this index
    mInsertIndex = ui->treeWidget->indexOfTopLevelItem(selectedItem);
    delete selectedItem;

    auto address = DbgValFromString(mGotoDialog->expressionText.toUtf8().constData());
    DbgCmdExec(QString("DisplayType %1, %2").arg(type.typeName, ToPtrString(address)));
}

void StructWidget::copyColumnSlot()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if(action == nullptr || !hasSelection)
        return;
    auto column = action->objectName().toInt();
    auto text = selectedItem->text(column);
    text = QTextDocumentFragment::fromHtml(text).toPlainText();
    if(!text.isEmpty())
        Bridge::CopyToClipboard(text);
}
