#pragma once

#include <QDialog>
#include <QHeaderView>
#include "Configuration.h"

namespace Ui
{
    class ShortcutsDialog;
}

class ShortcutsDialog : public QDialog
{
    Q_OBJECT
    Configuration::Shortcut currentShortcut;
    int currentRow;

public:
    explicit ShortcutsDialog(QWidget* parent = nullptr);
    ~ShortcutsDialog();

protected slots:
    void syncTextfield();
    void updateShortcut();

private slots:
    void on_btnSave_clicked();
    void on_btnClearShortcut_clicked();
    void rejectedSlot();
    void on_FilterTextChanged(const QString & actionName);

private:
    Ui::ShortcutsDialog* ui;
    QMap<QString, Configuration::Shortcut> ShortcutsBackup;
    void filterShortcutsByName(const QString & nameFilter, QMap<QString, Configuration::Shortcut> & mapToFill);
    void showShortcutsFiltered(const QString & actionName);
};
