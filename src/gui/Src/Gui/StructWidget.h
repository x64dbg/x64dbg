#pragma once

#include <QWidget>
#include "Bridge.h"
#include "ActionHelpers.h"

class MenuBuilder;
class GotoDialog;

namespace Ui
{
    class StructWidget;
}

class StructWidget : public QWidget, public ActionHelper<StructWidget>
{
    Q_OBJECT

public:
    explicit StructWidget(QWidget* parent = nullptr);
    ~StructWidget() override;
    void saveWindowSettings();
    void loadWindowSettings();

public slots:
    void colorsUpdatedSlot();
    void fontsUpdatedSlot();
    void shortcutsUpdatedSlot();

    void typeAddNodeSlot(void* parent, const TYPEDESCRIPTOR* type);
    void typeClearSlot();
    void typeVisitSlot(QString typeName, duint addr);
    void typeUpdateWidgetSlot();
    void dbgStateChangedSlot(DBGSTATE state);

private:
    Ui::StructWidget* ui;
    MenuBuilder* mMenuBuilder;
    GotoDialog* mGotoDialog = nullptr;
    QColor mTextColor;
    int mInsertIndex = -1;
    class QTreeWidgetItem* mScrollItem = nullptr;

    void setupColumns();
    void setupContextMenu();
    QString highlightTypeName(QString name) const;
    duint selectedValue() const;
    class QTreeWidgetItem* typeAddNode(class QTreeWidgetItem* parent, const TYPEDESCRIPTOR* type);

    enum
    {
        ColField,
        ColOffset,
        ColAddress,
        ColSize,
        ColValue,
    };

private slots:
    void on_treeWidget_customContextMenuRequested(const QPoint & pos);

    void followDumpSlot();
    void followValueDumpSlot();
    void followValueDisasmSlot();
    void clearSlot();
    void removeSlot();
    void displayTypeSlot();
    void loadJsonSlot();
    void parseFileSlot();
    void reloadTypeSlot();
    void copyColumnSlot();
};
