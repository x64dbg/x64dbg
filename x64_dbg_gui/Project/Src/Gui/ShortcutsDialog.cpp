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

    for(unsigned int i=0; i<numShortcuts; i++)
    {
        QTableWidgetItem* shortcutName = new QTableWidgetItem(Config()->getShortcut(static_cast<XH::ShortcutId>(i)).Name);
        QTableWidgetItem* shortcutKey = new QTableWidgetItem(Config()->getShortcut(static_cast<XH::ShortcutId>(i)).Hotkey.toString(QKeySequence::NativeText));

        tbl->setItem(i,0,shortcutName);
        tbl->setItem(i,1,shortcutKey);
    }

    connect(ui->tblShortcuts,SIGNAL(clicked(QModelIndex)),this,SLOT(syncTextfield()));


    shortcutfield = new ShortcutEdit(this);

    ui->horizontalLayout->addWidget(shortcutfield);
    connect(shortcutfield,SIGNAL(askForSave()),this,SLOT(updateShortcut()));

}
void ShortcutsDialog::updateShortcut()
{
    const QKeySequence newKey = shortcutfield->getKeysequence();
    if(newKey != currentShortcut.Hotkey)
    {
        bool good=true;
        foreach(XH::Shortcut S,Config()->Shortcuts )
        {
            if((S.Hotkey == newKey) && (S.Id != currentShortcut.Id))
            {
                good=false;
                break;
            }
        }
        if(good)
        {
            Config()->setShortcut(currentShortcut.Id,newKey);
            ui->tblShortcuts->item(currentRow,1)->setText(newKey.toString(QKeySequence::NativeText));
            shortcutfield->setErrorState(false);
        }
        else
        {
            shortcutfield->setErrorState(true);
        }
    }
}

void ShortcutsDialog::syncTextfield()
{
    QModelIndexList indexes = ui->tblShortcuts->selectionModel()->selectedRows();
    if(indexes.count()<1)
        return;
    currentRow = indexes.at(0).row();
    currentShortcut = Config()->getShortcut(static_cast<XH::ShortcutId>(indexes.at(0).row()));
    shortcutfield->setErrorState(false);
    shortcutfield->setText(currentShortcut.Hotkey.toString(QKeySequence::NativeText));

}

ShortcutsDialog::~ShortcutsDialog()
{
    delete ui;
}
