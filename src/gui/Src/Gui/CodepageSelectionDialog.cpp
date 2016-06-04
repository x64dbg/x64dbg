#include "CodepageSelectionDialog.h"
#include "ui_CodepageSelectionDialog.h"
#include <QTextCodec>

CodepageSelectionDialog::CodepageSelectionDialog(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::CodepageSelectionDialog)
{
    ui->setupUi(this);
    setModal(true);
    setFixedSize(this->size()); //fixed size
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint | Qt::MSWindowsFixedSizeDialogHint);
    setWindowIcon(QIcon(":/icons/images/codepage.png"));
    for(auto & name : QTextCodec::availableCodecs())
    {
        auto codec = QTextCodec::codecForName(name);
        if(!codec)
            continue;
        ui->listCodepages->addItem(name);
        mCodepages.append(codec->name());
    }
    ui->listCodepages->setCurrentRow(0);
}

CodepageSelectionDialog::~CodepageSelectionDialog()
{
    delete ui;
}

QByteArray CodepageSelectionDialog::getSelectedCodepage()
{
    return mCodepages[ui->listCodepages->currentRow()];
}
