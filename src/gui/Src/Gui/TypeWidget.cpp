#include "TypeWidget.h"
#include "Configuration.h"
#include "RichTextItemDelegate.h"
#include "StringUtil.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QMessageBox>

TypeWidget::TypeWidget(QWidget* parent)
    : QTreeWidget(parent)
{
    // Set up tree widget properties matching StructWidget.ui
    setContextMenuPolicy(Qt::CustomContextMenu);
    setAlternatingRowColors(true);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setIndentation(15);
    header()->setCascadingSectionResizes(false);

    // Set up columns
    QStringList headers;
    headers << tr("Field") << tr("Offset") << tr("Address") << tr("Size") << tr("Value");
    setHeaderLabels(headers);

    // Set item delegate for rich text rendering
    setItemDelegate(new RichTextItemDelegate(&mTextColor, this));

    // Set uniform row height to prevent spacing changes when content changes
    setUniformRowHeights(true);

    // Connect only to configuration signals (not Bridge type signals)
    connect(Config(), SIGNAL(colorsUpdated()), this, SLOT(colorsUpdatedSlot()));
    connect(Config(), SIGNAL(fontsUpdated()), this, SLOT(fontsUpdatedSlot()));

    // Apply styles and fonts first, then setup columns
    colorsUpdatedSlot();
    fontsUpdatedSlot();

    auto charWidth = fontMetrics().width(' ');
    setColumnWidth(ColField, 4 + charWidth * 50);
    setColumnWidth(ColOffset, 6 + charWidth * 7);
    setColumnWidth(ColAddress, 6 + charWidth * sizeof(duint) * 2);
    setColumnWidth(ColSize, 4 + charWidth * 6);

    // NOTE: Trick to display the expander icons in the second column
    // Reference: https://stackoverflow.com/a/25887454/1806760
    // header()->moveSection(ColField, ColOffset);
}

void TypeWidget::clearTypes()
{
    clear();
    mScrollItem = nullptr;
    mInsertIndex = -1;
}

QTreeWidgetItem* TypeWidget::typeAddNode(QTreeWidgetItem* parent, const TYPEDESCRIPTOR* type)
{
    // Disable updates until the next typeUpdateWidget()
    setUpdatesEnabled(false);

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
    for(int i = 0; i < columnCount(); i++)
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
            insertTopLevelItem(mInsertIndex, item);

            mInsertIndex = -1;
        }
        else
        {
            item = new QTreeWidgetItem(this, text);
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
    return item;
}

void TypeWidget::saveWindowSettings(const QString & settingSection)
{
    auto saveColumn = [&](int column)
    {
        auto settingName = QString("TypeWidgetColumn%1").arg(column);
        BridgeSettingSetUint(settingSection.toUtf8().constData(), settingName.toUtf8().constData(), columnWidth(column));
    };
    auto columnCount = this->columnCount();
    for(int i = 0; i < columnCount; i++)
        saveColumn(i);
}

void TypeWidget::loadWindowSettings(const QString & settingSection)
{
    auto loadColumn = [&](int column)
    {
        auto settingName = QString("TypeWidgetColumn%1").arg(column);
        duint width = 0;
        if(BridgeSettingGetUint(settingSection.toUtf8().constData(), settingName.toUtf8().constData(), &width))
            setColumnWidth(column, width);
    };
    auto columnCount = this->columnCount();
    for(int i = 0; i < columnCount; i++)
        loadColumn(i);
}

void TypeWidget::colorsUpdatedSlot()
{
    mTextColor = ConfigColor("StructTextColor");
    auto background = ConfigColor("StructBackgroundColor");
    auto altBackground = ConfigColor("StructAlternateBackgroundColor");
    auto style = QString("QTreeWidget { background-color: %1; alternate-background-color: %2; }").arg(background.name(), altBackground.name());
    setStyleSheet(style);
}

void TypeWidget::fontsUpdatedSlot()
{
    auto font = ConfigFont("AbstractTableView");
    setFont(font);
    header()->setFont(font);
}

void TypeWidget::updateValuesSlot()
{
    setUpdatesEnabled(false);
    for(QTreeWidgetItemIterator it(this); *it; ++it)
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
                    std::reverse((char*)&data, (char*)&data + (type.type.sizeBits / 8));
                valueStr = QString("0x%1, %2").arg(ToHexString(data)).arg(data);
            }
            else if(type.type.addr)
                valueStr = "???";
        }
        item->setText(ColValue, valueStr);
    }
    setUpdatesEnabled(true);

    // Scroll to the item the user selected
    if(mScrollItem)
    {
        scrollToItem(mScrollItem, QAbstractItemView::PositionAtTop);
        setCurrentItem(mScrollItem);
        mScrollItem = nullptr;
    }
}

QString TypeWidget::highlightTypeName(QString name) const
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
