#include "HexEditDialog.h"
#include "ui_HexEditDialog.h"
#include "Configuration.h"
#include "Bridge.h"

HexEditDialog::HexEditDialog(QWidget* parent) : QDialog(parent), ui(new Ui::HexEditDialog)
{
    ui->setupUi(this);
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
    setWindowFlags(Qt::Dialog | Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::MSWindowsFixedSizeDialogHint);
#endif
    setFixedSize(this->size()); //fixed size
    setModal(true); //modal window

    //setup text fields
    QFont font("Monospace", 8, QFont::Normal, false);
    font.setFixedPitch(true);
    font.setStyleHint(QFont::Monospace);
    ui->lineEditAscii->setFont(font);
    ui->lineEditUnicode->setFont(font);
    ui->chkEntireBlock->hide();
    connect(Bridge::getBridge(), SIGNAL(repaintGui()), this, SLOT(updateStyle()));
    updateStyle();

    //setup hex editor
    mHexEdit = new QHexEdit(this);
    mHexEdit->setEditFont(ConfigFont("HexEdit"));
    mHexEdit->setHorizontalSpacing(6);
    mHexEdit->setOverwriteMode(true);
    mHexEdit->setTextColor(ConfigColor("HexEditTextColor"));
    mHexEdit->setWildcardColor(ConfigColor("HexEditWildcardColor"));
    mHexEdit->setBackgroundColor(ConfigColor("HexEditBackgroundColor"));
    mHexEdit->setSelectionColor(ConfigColor("HexEditSelectionColor"));
    connect(mHexEdit, SIGNAL(dataChanged()), this, SLOT(dataChangedSlot()));
    ui->scrollArea->setWidget(mHexEdit);
    mHexEdit->widget()->setFocus();
    mHexEdit->setTabOrder(ui->btnUnicode2Hex, mHexEdit->widget());
}

HexEditDialog::~HexEditDialog()
{
    delete ui;
}

void HexEditDialog::showEntireBlock(bool show)
{
    if(show)
        ui->chkEntireBlock->show();
    else
        ui->chkEntireBlock->hide();
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
}

void HexEditDialog::on_btnAscii2Hex_clicked()
{
    QString text = ui->lineEditAscii->text();
    QByteArray data;
    for(int i = 0; i < text.length(); i++)
        data.append(text[i].toLatin1());
    if(ui->chkKeepSize->isChecked()) //keep size
    {
        int dataSize = mHexEdit->data().size();
        if(dataSize < data.size())
            data.resize(dataSize);
        else if(dataSize > data.size())
            data.append(QByteArray(dataSize - data.size(), 0));
    }
    mHexEdit->setData(data);
}

void HexEditDialog::on_btnUnicode2Hex_clicked()
{
    QByteArray data =  QTextCodec::codecForName("UTF-16")->makeEncoder(QTextCodec::IgnoreHeader)->fromUnicode(ui->lineEditUnicode->text());
    if(ui->chkKeepSize->isChecked()) //keep size
    {
        int dataSize = mHexEdit->data().size();
        if(dataSize < data.size())
            data.resize(dataSize);
        else if(dataSize > data.size())
            data.append(QByteArray(dataSize - data.size(), 0));
    }
    mHexEdit->setData(data);
}

void HexEditDialog::on_chkKeepSize_toggled(bool checked)
{
    mHexEdit->setKeepSize(checked);
}

void HexEditDialog::dataChangedSlot()
{
    QByteArray data = mHexEdit->data();
    QString ascii;
    for(int i = 0; i < data.size(); i++)
    {
        QChar ch(data.constData()[i]);
        if(ch.isPrint())
            ascii += ch.toLatin1();
        else
            ascii += '.';
    }
    QString unicode;
    for(int i = 0, j = 0; i < data.size(); i += 2, j++)
    {
        QChar wch(((wchar_t*)data.constData())[j]);
        if(wch.isPrint())
            unicode += wch;
        else
            unicode += '.';
    }
    ui->lineEditAscii->setText(ascii);
    ui->lineEditUnicode->setText(unicode);
}
