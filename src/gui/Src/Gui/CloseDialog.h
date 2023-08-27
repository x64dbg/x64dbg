#pragma once

#include <QDialog>

class QCloseEvent;

namespace Ui
{
    class CloseDialog;
}

class CloseDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CloseDialog(QWidget* parent = nullptr);
    ~CloseDialog();
    void closeEvent(QCloseEvent* event);
    void allowClose();

private:
    Ui::CloseDialog* ui;
    bool bCanClose;
};
