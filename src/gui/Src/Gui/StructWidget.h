#ifndef STRUCTWIDGET_H
#define STRUCTWIDGET_H

#include <QWidget>

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

private:
    Ui::StructWidget* ui;

    void showTree();
};

#endif // STRUCTWIDGET_H
