#ifndef CLOSEDIALOG_H
#define CLOSEDIALOG_H

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
    explicit CloseDialog(QWidget* parent = 0);
    ~CloseDialog();
    void closeEvent(QCloseEvent* event);
    void allowClose();

private:
    Ui::CloseDialog* ui;
    bool bCanClose;
};

#endif // CLOSEDIALOG_H
