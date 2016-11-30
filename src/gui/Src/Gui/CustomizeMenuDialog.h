#ifndef CUSTOMIZEMENUDIALOG_H
#define CUSTOMIZEMENUDIALOG_H

#include <QDialog>

namespace Ui
{
    class CustomizeMenuDialog;
}

class CustomizeMenuDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CustomizeMenuDialog(QWidget* parent = 0);
    ~CustomizeMenuDialog();

public slots:
    void onOk();
    void onDisselectAll();

private:
    Ui::CustomizeMenuDialog* ui;
};

#endif // CUSTOMIZEMENUDIALOG_H
