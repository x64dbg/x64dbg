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
    explicit StructWidget(QWidget* parent = 0);
    ~StructWidget();

public slots:
    void colorsUpdatedSlot();
    void fontsUpdatedSlot();
    void shortcutsUpdatedSlot();

    void typeAddNode(void* parent, const TYPEDESCRIPTOR* type);
    void typeClear();
    void typeUpdateWidget();
    void dbgStateChangedSlot(DBGSTATE state);

private:
    Ui::StructWidget* ui;
    MenuBuilder* mMenuBuilder;
    GotoDialog* mGotoDialog = nullptr;

    void setupColumns();
    void setupContextMenu();
    QString highlightTypeName(QString name) const;

private slots:
    void on_treeWidget_customContextMenuRequested(const QPoint & pos);

    void followDumpSlot();
    void clearSlot();
    void removeSlot();
    void visitSlot();
    void loadJsonSlot();
    void parseFileSlot();
    void changeAddrSlot();
    void refreshSlot();
};
