#include "ShortcutsDialog.h"
#include "ui_ShortcutsDialog.h"




ShortcutsDialog::ShortcutsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ShortcutsDialog)
{
    ui->setupUi(this);
    //set window flags
    setModal(true);
    setWindowFlags(Qt::Dialog | Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::MSWindowsFixedSizeDialogHint);
    setFixedSize(this->size()); //fixed size

    // x64 has no model-view-controler pattern
    QStringList tblHeader;
    tblHeader << "Instruction" << "Shortcut";
    QTableWidget* tbl = ui->tblShortcuts;
    tbl->setColumnCount(2);
    tbl->verticalHeader()->setVisible(false);
    tbl->setHorizontalHeaderLabels(tblHeader);
    tbl->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tbl->setSelectionBehavior(QAbstractItemView::SelectRows);
    tbl->setSelectionMode(QAbstractItemView::SingleSelection);
    tbl->setShowGrid(false);
    tbl->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
    tbl->verticalHeader()->setDefaultSectionSize(15);

    const unsigned int numShortcuts = Config()->Shortcuts.count();
    tbl->setRowCount(numShortcuts);
    int j=0;
    for(QMap<QString, Configuration::Shortcut>::iterator i=Config()->Shortcuts.begin(); i!=Config()->Shortcuts.end(); ++i,j++)
    {
        QTableWidgetItem* shortcutName = new QTableWidgetItem(i.value().Name);
        QTableWidgetItem* shortcutKey = new QTableWidgetItem(i.value().Hotkey.toString(QKeySequence::NativeText));
        tbl->setItem(j, 0, shortcutName);
        tbl->setItem(j, 1, shortcutKey);
    }

    connect(ui->tblShortcuts, SIGNAL(clicked(QModelIndex)), this, SLOT(syncTextfield()));

    connect(ui->shortcutEdit, SIGNAL(askForSave()), this, SLOT(updateShortcut()));

}
void ShortcutsDialog::updateShortcut()
{
    const QKeySequence newKey = ui->shortcutEdit->getKeysequence();
    if(newKey != currentShortcut.Hotkey)
    {
        bool good=true;
        foreach(Configuration::Shortcut S, Config()->Shortcuts)
        {
            if(!newKey.isEmpty() && S.Hotkey == newKey && S.Name != currentShortcut.Name)
            {
                good=false;
                break;
            }
        }
        if(good)
        {
            for(QMap<QString, Configuration::Shortcut>::iterator i=Config()->Shortcuts.begin(); i!=Config()->Shortcuts.end(); ++i)
            {
                if(i.value().Name == currentShortcut.Name)
                {
                    Config()->setShortcut(i.key(), newKey);
                    break;
                }
            }
            QString keyText = "";
            if(!newKey.isEmpty())
                keyText = newKey.toString(QKeySequence::NativeText);
            ui->tblShortcuts->item(currentRow, 1)->setText(keyText);
            ui->shortcutEdit->setErrorState(false);
        }
        else
        {
            ui->shortcutEdit->setErrorState(true);
        }
    }
}

void ShortcutsDialog::syncTextfield()
{
    QModelIndexList indexes = ui->tblShortcuts->selectionModel()->selectedRows();
    if(indexes.count()<1)
        return;
    currentRow = indexes.at(0).row();
    for(QMap<QString, Configuration::Shortcut>::iterator i=Config()->Shortcuts.begin(); i!=Config()->Shortcuts.end(); ++i)
    {
        if(i.value().Name == ui->tblShortcuts->item(currentRow, 0)->text())
        {
            currentShortcut = i.value();
            break;
        }
    }
    ui->shortcutEdit->setErrorState(false);
    ui->shortcutEdit->setText(currentShortcut.Hotkey.toString(QKeySequence::NativeText));

}

ShortcutsDialog::~ShortcutsDialog()
{
    delete ui;
}

void ShortcutsDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    QMessageBox msg(QMessageBox::Information, "Information", "Shortcuts updated!\n\nYou may need to restart the debugger for all changes to take in effect.");
    msg.setWindowIcon(QIcon(":/icons/images/information.png"));
    msg.setParent(this, Qt::Dialog);
    msg.setWindowFlags(msg.windowFlags()&(~Qt::WindowContextHelpButtonHint));
    msg.exec();
}
