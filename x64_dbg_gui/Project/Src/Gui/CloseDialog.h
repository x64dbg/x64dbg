#ifndef CLOSEDIALOG_H
#define CLOSEDIALOG_H

#include <QDialog>
#include <QCloseEvent>

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

private:
    Ui::CloseDialog* ui;
};

#endif // CLOSEDIALOG_H
