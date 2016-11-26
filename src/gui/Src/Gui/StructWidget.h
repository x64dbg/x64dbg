#ifndef STRUCTWIDGET_H
#define STRUCTWIDGET_H

#include <QWidget>
#include "Bridge.h"

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
    void typeAddNode(void* parent, const TYPEDESCRIPTOR* type);
    void typeClear();
    void typeUpdateWidget();

private:
    Ui::StructWidget* ui;

    void setupColumns();
};

#endif // STRUCTWIDGET_H
