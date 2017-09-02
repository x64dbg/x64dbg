#include <QTextCodec>
#include "HexEditDialog.h"
#include "QHexEdit/QHexEdit.h"
#include "ui_HexEditDialog.h"
#include "Configuration.h"
#include "Bridge.h"
#include "CodepageSelectionDialog.h"
#include "LineEditDialog.h"

HexEditDialog::HexEditDialog(QWidget* parent) : QDialog(parent), ui(new Ui::HexEditDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint | Qt::MSWindowsFixedSizeDialogHint);
    setModal(true); //modal window

    //setup text fields
    ui->lineEditAscii->setEncoding(QTextCodec::codecForName("System"));
    ui->lineEditUnicode->setEncoding(QTextCodec::codecForName("UTF-16"));

    ui->chkEntireBlock->hide();

    mDataInitialized = false;

    //setup hex editor
    mHexEdit = new QHexEdit(this);
    mHexEdit->setEditFont(ConfigFont("HexEdit"));
    mHexEdit->setHorizontalSpacing(6);
    mHexEdit->setOverwriteMode(true);
    ui->scrollArea->setWidget(mHexEdit);
    mHexEdit->widget()->setFocus();
    connect(mHexEdit, SIGNAL(dataChanged()), this, SLOT(dataChangedSlot()));
    connect(mHexEdit, SIGNAL(dataEdited()), this, SLOT(dataEditedSlot()));

    connect(Config(), SIGNAL(colorsUpdated()), this, SLOT(updateStyle()));
    connect(Bridge::getBridge(), SIGNAL(repaintGui()), this, SLOT(updateStyle()));

    updateStyle();
    updateCodepage();
    Config()->setupWindowPos(this);
}

HexEditDialog::~HexEditDialog()
{
    Config()->saveWindowPos(this);
    delete ui;
}

void HexEditDialog::showEntireBlock(bool show)
{
    if(show)
        ui->chkEntireBlock->show();
    else
        ui->chkEntireBlock->hide();
}

void HexEditDialog::showKeepSize(bool show)
{
    if(show)
        ui->chkKeepSize->show();
    else
        ui->chkKeepSize->hide();
}

void HexEditDialog::updateCodepage()
{
    duint lastCodepage;
    auto allCodecs = QTextCodec::availableCodecs();
    if(!BridgeSettingGetUint("Misc", "LastCodepage", &lastCodepage) || lastCodepage >= duint(allCodecs.size()))
        return;
    ui->lineEditCodepage->setEncoding(QTextCodec::codecForName(allCodecs.at(lastCodepage)));
    ui->lineEditCodepage->setData(mHexEdit->data());
    ui->labelLastCodepage->setText(QString(allCodecs.at(lastCodepage).constData()));
}

void HexEditDialog::updateCodepage(const QByteArray & name)
{
    ui->lineEditCodepage->setEncoding(QTextCodec::codecForName(name));
    ui->lineEditCodepage->setData(mHexEdit->data());
    ui->labelLastCodepage->setText(QString(name));
}

bool HexEditDialog::entireBlock()
{
    return ui->chkEntireBlock->isChecked();
}

void HexEditDialog::updateStyle()
{
    QString style = QString("QLineEdit { border-style: outset; border-width: 1px; border-color: %1; color: %1; background-color: %2 }").arg(ConfigColor("HexEditTextColor").name(), ConfigColor("HexEditBackgroundColor").name());
    ui->lineEditAscii->setStyleSheet(style);
    ui->lineEditUnicode->setStyleSheet(style);
    ui->lineEditCodepage->setStyleSheet(style);

    mHexEdit->setTextColor(ConfigColor("HexEditTextColor"));
    mHexEdit->setWildcardColor(ConfigColor("HexEditWildcardColor"));
    mHexEdit->setBackgroundColor(ConfigColor("HexEditBackgroundColor"));
    mHexEdit->setSelectionColor(ConfigColor("HexEditSelectionColor"));
}

void HexEditDialog::on_chkKeepSize_toggled(bool checked)
{
    mHexEdit->setKeepSize(checked);
    ui->lineEditAscii->setKeepSize(checked);
    ui->lineEditUnicode->setKeepSize(checked);
    ui->lineEditCodepage->setKeepSize(checked);
}

void HexEditDialog::dataChangedSlot()
{
    // Allows initialization of the data by calling setData() on mHexEdit.
    if(!mDataInitialized)
    {
        QByteArray data = mHexEdit->data();
        ui->lineEditAscii->setData(data);
        ui->lineEditUnicode->setData(data);
        ui->lineEditCodepage->setData(data);
        mDataInitialized = true;
    }
}

void HexEditDialog::dataEditedSlot()
{
    QByteArray data = mHexEdit->data();
    ui->lineEditAscii->setData(data);
    ui->lineEditUnicode->setData(data);
    ui->lineEditCodepage->setData(data);
}

void HexEditDialog::on_lineEditAscii_dataEdited()
{
    QByteArray data = ui->lineEditAscii->data();
    data = resizeData(data);
    ui->lineEditUnicode->setData(data);
    ui->lineEditCodepage->setData(data);
    mHexEdit->setData(data);
}

void HexEditDialog::on_lineEditUnicode_dataEdited()
{
    QByteArray data = ui->lineEditUnicode->data();
    data = resizeData(data);
    ui->lineEditAscii->setData(data);
    ui->lineEditCodepage->setData(data);
    mHexEdit->setData(data);
}

void HexEditDialog::on_lineEditCodepage_dataEdited()
{
    QByteArray data = ui->lineEditCodepage->data();
    data = resizeData(data);
    ui->lineEditAscii->setData(data);
    ui->lineEditUnicode->setData(data);
    mHexEdit->setData(data);
}

QByteArray HexEditDialog::resizeData(QByteArray & data)
{
    // truncate or pad the data
    if(mHexEdit->keepSize())
    {
        int dataSize = mHexEdit->data().size();
        int data_size = data.size();
        if(dataSize < data_size)
            data.resize(dataSize);
        else if(dataSize > data_size)
            data.append(QByteArray(dataSize - data_size, 0));
    }

    return data;
}

void HexEditDialog::on_btnCodepage_clicked()
{
    CodepageSelectionDialog codepageDialog(this);
    if(codepageDialog.exec() != QDialog::Accepted)
        return;
    updateCodepage(codepageDialog.getSelectedCodepage());
}
