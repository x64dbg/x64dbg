#pragma once

#include <QDialog>

class StdIconSearchListView;
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
    explicit AttachDialog(QWidget* parent = nullptr);
    ~AttachDialog();

private slots:
    void on_btnAttach_clicked();
    void on_btnFindWindow_clicked();
    void refresh();
    void processListContextMenu(QMenu* menu);

private:
    Ui::AttachDialog* ui;
    StdIconSearchListView* mSearchListView;
    QAction* mAttachAction;
    QAction* mRefreshAction;

    enum
    {
        ColPid,
        ColName,
        ColTitle,
        ColPath,
        ColCommandLine,
    };
};
