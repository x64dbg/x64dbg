#ifndef STRUCTWIDGET_H
#define STRUCTWIDGET_H

#include <QWidget>
#include "Bridge.h"

class MenuBuilder;
class GotoDialog;

namespace Ui
{
    class StructWidget;
}

class StructWidget : public QWidget
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

private:
    Ui::StructWidget* ui;
    MenuBuilder* mMenuBuilder;
    GotoDialog* mGotoDialog = nullptr;

    void setupColumns();
    void setupContextMenu();

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

protected:
#include "ActionHelpers.h"
};

#endif // STRUCTWIDGET_H
