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
    connect(ui->btnCodePage2, SIGNAL(clicked()), this, SLOT(on_btnCodepage_clicked()));

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
    ui->chkEntireBlock->setVisible(show);
}

void HexEditDialog::showKeepSize(bool show)
{
    ui->chkKeepSize->setVisible(show);
}

void HexEditDialog::isDataCopiable(bool copyDataEnabled)
{
    if(copyDataEnabled == false)
        ui->tabModeSelect->removeTab(2); //This can't be undone!
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
    ui->labelLastCodepage2->setText(ui->labelLastCodepage->text());
}

void HexEditDialog::updateCodepage(const QByteArray & name)
{
    ui->lineEditCodepage->setEncoding(QTextCodec::codecForName(name));
    ui->lineEditCodepage->setData(mHexEdit->data());
    ui->labelLastCodepage->setText(QString(name));
    ui->labelLastCodepage2->setText(ui->labelLastCodepage->text());
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
        checkDataRepresentable(0);
        mDataInitialized = true;
    }
}

void HexEditDialog::dataEditedSlot()
{
    QByteArray data = mHexEdit->data();
    ui->lineEditAscii->setData(data);
    ui->lineEditUnicode->setData(data);
    ui->lineEditCodepage->setData(data);
    checkDataRepresentable(0);
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
    checkDataRepresentable(3);
    updateCodepage(codepageDialog.getSelectedCodepage());
}

bool HexEditDialog::checkDataRepresentable(int mode)
{
    QTextCodec* codec;
    QLabel* label;
    if(mode == 1)
    {
        codec = QTextCodec::codecForName("System");
        label = ui->labelWarningCodepageASCII;
    }
    else if(mode == 2)
    {
        codec = QTextCodec::codecForName("UTF-16");
        label = ui->labelWarningCodepageUTF;
    }
    else if(mode == 3)
    {
        codec = QTextCodec::codecForName(ui->labelLastCodepage->text().toLatin1());
        label = ui->labelWarningCodepage;
    }
    else if(mode == 4)
    {
        codec = QTextCodec::codecForName(ui->labelLastCodepage->text().toLatin1());
        label = ui->labelWarningCodepageString;
    }
    else
    {
        bool isRepresentable;
        isRepresentable = checkDataRepresentable(1);
        isRepresentable = checkDataRepresentable(2) && isRepresentable;
        isRepresentable = checkDataRepresentable(3) && isRepresentable;
        isRepresentable = checkDataRepresentable(4) && isRepresentable;
        return isRepresentable;
    }
    if(codec == nullptr)
    {
        label->show();
        return false;
    }
    QString test;
    QByteArray original = mHexEdit->data();
    QTextCodec::ConverterState converter(QTextCodec::IgnoreHeader);
    test = codec->toUnicode(original);
    QByteArray test2;
    test2 = codec->fromUnicode(test.constData(), test.size(), &converter);
    if(test2.size() < original.size() || memcmp(test2.constData(), original.constData(), original.size()) != 0)
    {
        label->show();
        return false;
    }
    else
    {
        label->hide();
        return true;
    }
}
