#include "CodepageSelectionDialog.h"
#include "ui_CodepageSelectionDialog.h"
#include "Bridge.h"
#include <QTextCodec>
#include "MiscUtil.h"

CodepageSelectionDialog::CodepageSelectionDialog(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::CodepageSelectionDialog)
{
    ui->setupUi(this);
    setModal(true);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint | Qt::MSWindowsFixedSizeDialogHint);
    setWindowIcon(DIcon("codepage"));
    for(auto & name : QTextCodec::availableCodecs())
    {
        auto codec = QTextCodec::codecForName(name);
        if(!codec)
            continue;
        ui->listCodepages->addItem(name);
        mCodepages.append(codec->name());
    }
    duint setting;
    if(BridgeSettingGetUint("Misc", "LastCodepage", &setting) && setting < duint(ui->listCodepages->count()))
        ui->listCodepages->setCurrentRow(setting);
    else
        ui->listCodepages->setCurrentRow(0);
}

CodepageSelectionDialog::~CodepageSelectionDialog()
{
    BridgeSettingSetUint("Misc", "LastCodepage", ui->listCodepages->currentRow());
    delete ui;
}

QByteArray CodepageSelectionDialog::getSelectedCodepage()
{
    return mCodepages[ui->listCodepages->currentRow()];
}
