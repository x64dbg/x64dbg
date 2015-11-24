#ifndef ATTACHDIALOG_H
#define ATTACHDIALOG_H

#include <QDialog>

namespace Ui
{
class AttachDialog;
}

class AttachDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AttachDialog(QWidget* parent = 0);
    ~AttachDialog();

private slots:
    void on_btnAttach_clicked();
    void refresh();
    void processListContextMenu(const QPoint & pos);

private:
    Ui::AttachDialog* ui;
    QAction* mAttachAction;
    QAction* mRefreshAction;
};

#endif // ATTACHDIALOG_H
