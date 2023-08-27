#pragma once

#include <QDialog>

namespace Ui
{
    class CustomizeMenuDialog;
}

class CustomizeMenuDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CustomizeMenuDialog(QWidget* parent = nullptr);
    ~CustomizeMenuDialog();

public slots:
    void onOk();
    void onDisselectAll();

private:
    Ui::CustomizeMenuDialog* ui;
};
