#ifndef SHORTCUTSDIALOG_H
#define SHORTCUTSDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include "ShortcutEdit.h"
#include "Configuration.h"

namespace Ui
{
class ShortcutsDialog;
}

class ShortcutsDialog : public QDialog
{
    Q_OBJECT
    QTableWidget* tbl;
    Configuration::Shortcut currentShortcut;
    int currentRow;
    bool editLock;
public:
    explicit ShortcutsDialog(QWidget *parent = 0);
    ~ShortcutsDialog();
signals:


protected slots:
    void syncTextfield();
    void updateShortcut();

private slots:
    void on_btnSave_clicked();
    void rejectedSlot();

private:
    Ui::ShortcutsDialog *ui;
    QMap<QString, Configuration::Shortcut> ShortcutsBackup;
};

#endif // SHORTCUTSDIALOG_H
