#include "ShortcutsDialog.h"
#include "ui_ShortcutsDialog.h"
#include "ShortcutEdit.h"

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
    tblHeader << "Instruction" << "Description" << "Shortcut";
    QTableWidget* tbl = ui->tblShortcuts;
    tbl->setColumnCount(3);
    tbl->verticalHeader()->setVisible(false);
    tbl->setHorizontalHeaderLabels(tblHeader);
    tbl->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tbl->setSelectionBehavior(QAbstractItemView::SelectRows);
    tbl->setSelectionMode(QAbstractItemView::SingleSelection);
    tbl->setShowGrid(false);

    ShortcutEdit *SH = new ShortcutEdit(this);

    ui->horizontalLayout->addWidget(SH);

}

ShortcutsDialog::~ShortcutsDialog()
{
    delete ui;
}
