#ifndef STRUCTWIDGET_H
#define STRUCTWIDGET_H

#include <QWidget>

namespace Ui {
class StructWidget;
}

class StructWidget : public QWidget
{
    Q_OBJECT

public:
    explicit StructWidget(QWidget *parent = nullptr);
    ~StructWidget();

private slots:
    void on_pushButtonParse_clicked();

    void on_plainTextEditDeclaration_textChanged();

private:
    Ui::StructWidget *ui;
};

#endif // STRUCTWIDGET_H
