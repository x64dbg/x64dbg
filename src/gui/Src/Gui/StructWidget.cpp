#include <QFileDialog>
#include <QTextDocumentFragment>

#include "Configuration.h"
#include "GotoDialog.h"
#include "DisplayTypeDialog.h"
#include "MenuBuilder.h"
#include "MiscUtil.h"
#include "StringUtil.h"
#include "StructWidget.h"

StructWidget::StructWidget(QWidget* parent)
    : TypeWidget(parent)
{
    connect(Bridge::getBridge(), &Bridge::typeAddNode, this, &StructWidget::typeAddNodeSlot);
    connect(Bridge::getBridge(), &Bridge::typeClear, this, &StructWidget::typeClearSlot);
    connect(Bridge::getBridge(), &Bridge::typeUpdateWidget, this, &TypeWidget::updateValuesSlot);
    connect(Bridge::getBridge(), &Bridge::typeVisit, this, &StructWidget::typeVisitSlot);
    connect(Bridge::getBridge(), &Bridge::dbgStateChanged, this, &StructWidget::dbgStateChangedSlot);
    connect(this, &QTreeWidget::customContextMenuRequested, this, &StructWidget::contextMenuRequestedSlot);
    setupContextMenu();
}

#define hasSelection !!selectedItems().count()
#define selectedItem selectedItems()[0]
#define selectedType selectedItem->data(0, Qt::UserRole).value<TypeDescriptor>().type

duint StructWidget::selectedValue() const
{
    if(!hasSelection)
        return 0;
    QStringList split = selectedItem->text(ColValue).split(',');
    if(split.length() < 1)
        return 0;
    return split[0].toULongLong(nullptr, 0);
}

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
    mMenuBuilder->addSeparator();
    mMenuBuilder->addAction(makeAction(DIcon("visitstruct"), tr("Display type"), SLOT(displayTypeSlot())));
    mMenuBuilder->addAction(makeDescAction(DIcon("structaddr"), tr("Reload type"), tr("Reload the type from the database and display it (at a different address)."), SLOT(reloadTypeSlot())), [this](QMenu*)
    {
        return hasSelection && !selectedItem->parent() && DbgIsDebugging();
    });
    mMenuBuilder->addAction(makeAction(DIcon("database-import"), tr("Load JSON"), SLOT(loadJsonSlot())));
    mMenuBuilder->addAction(makeAction(DIcon("source"), tr("Parse header"), SLOT(parseFileSlot())));
    mMenuBuilder->addAction(makeAction(DIcon("removestruct"), tr("Remove"), SLOT(removeSlot())), [this](QMenu*)
    {
        return hasSelection && !selectedItem->parent();
    });
    mMenuBuilder->addAction(makeAction(DIcon("eraser"), tr("Remove all"), SLOT(clearSlot())));
    mMenuBuilder->addAction(makeShortcutDescAction(DIcon("sync"), tr("&Refresh values"), tr("Quickly refresh the values, without reloading the type."), SLOT(typeUpdateWidgetSlot()), "ActionRefresh"));

    auto copyMenu = new MenuBuilder(this);
    for(int column = 0; column < columnCount(); column++)
    {
        auto action = makeAction(headerItem()->text(column), SLOT(copyColumnSlot()));
        action->setObjectName(QString("%1").arg(column));
        copyMenu->addAction(action, [this, column](QMenu*)
        {
            return hasSelection && !selectedItem->text(column).isEmpty();
        });
    }
    mMenuBuilder->addMenu(makeMenu(DIcon("copy"), tr("&Copy")), copyMenu);

    mMenuBuilder->loadFromConfig();
}

void StructWidget::typeAddNodeSlot(void* parent, const TYPEDESCRIPTOR* type)
{
    auto item = typeAddNode((QTreeWidgetItem*)parent, type);
    Bridge::getBridge()->setResult(BridgeResult::TypeAddNode, dsint(item));
}

void StructWidget::typeClearSlot()
{
    clearTypes();
    Bridge::getBridge()->setResult(BridgeResult::TypeClear);
}

void StructWidget::typeVisitSlot(QString typeName, duint addr)
{
    TYPEVISITDATA data = {};
    auto typeNameUtf8 = typeName.toUtf8();
    data.typeName = typeNameUtf8.constData();
    data.addr = addr;
    data.maxPtrDepth = -1;
    data.maxExpandDepth = -1;
    data.maxExpandArray = -1;
    data.createLabels = true;
    data.callback = [](void* parent, const TYPEDESCRIPTOR * type, void* userdata) -> void*
    {
        return ((StructWidget*)userdata)->typeAddNode((QTreeWidgetItem*)parent, type);
    };
    data.userdata = this;
    if(!DbgTypeVisit(&data))
    {
        SimpleErrorBox(this, tr("Error"), tr("Failed to visit type..."));
    }
    updateValuesSlot();
    GuiUpdateAllViews();
}

void StructWidget::dbgStateChangedSlot(DBGSTATE state)
{
    if(state == stopped)
        clearTypes();
}

void StructWidget::contextMenuRequestedSlot(const QPoint & pos)
{
    QMenu menu;
    mMenuBuilder->build(&menu);
    if(menu.actions().count())
        menu.exec(viewport()->mapToGlobal(pos));
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
    clearTypes();
}

void StructWidget::removeSlot()
{
    if(!hasSelection)
        return;
    delete selectedItem;
}

void StructWidget::displayTypeSlot()
{
    DisplayTypeDialog::pickType(this);
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
    mInsertIndex = indexOfTopLevelItem(selectedItem);
    delete selectedItem;

    auto address = DbgValFromString(mGotoDialog->expressionText.toUtf8().constData());
    typeVisitSlot(type.typeName, address);
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
