#ifndef ATTACHDIALOG_H
#define ATTACHDIALOG_H

#include <QDialog>

class SearchListView;
class QMenu;
class QAction;

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
    void on_btnFindWindow_clicked();
    void refresh();
    void processListContextMenu(QMenu* wMenu);

private:
    Ui::AttachDialog* ui;

    SearchListView* mSearchListView;

    QAction* mAttachAction;
    QAction* mRefreshAction;
};

#endif // ATTACHDIALOG_H
