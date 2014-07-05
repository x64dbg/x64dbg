#include "HexEditDialog.h"
#include "ui_HexEditDialog.h"
#include "QHexEdit/QHexEdit.h"

HexEditDialog::HexEditDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HexEditDialog)
{
    ui->setupUi(this);
    setModal(true);
    setWindowFlags(Qt::Dialog | Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::MSWindowsFixedSizeDialogHint);
    setFixedSize(this->size()); //fixed size
    setModal(true); //modal window
    mHexEdit = new QHexEdit(this);
    mHexEdit->setHorizontalSpacing(6);
    connect(mHexEdit, SIGNAL(dataChanged()), this, SLOT(dataChangedSlot()));
    mHexEdit->setData(QString("11223344556677889900aabbccddeeff"));
    ui->scrollArea->setWidget(mHexEdit);

    QFont font("Monospace", 8);
    font.setFixedPitch(true);
    font.setStyleHint(QFont::Monospace);
    ui->lineEditAscii->setFont(font);
    ui->lineEditUnicode->setFont(font);
    ui->lineEditAscii->setFocus();
}

HexEditDialog::~HexEditDialog()
{
    delete ui;
}

void HexEditDialog::on_btnAscii2Hex_clicked()
{
    QString text = ui->lineEditAscii->text();
    QByteArray data;
    for(int i=0; i<text.length(); i++)
        data.append(text[i].toAscii());
    if(ui->chkKeepSize->isChecked()) //keep size
    {
        int dataSize = mHexEdit->data().size();
        if(dataSize < data.size())
            data.resize(dataSize);
        else if(dataSize > data.size())
            data.append(QByteArray(dataSize-data.size(), 0));
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
            data.append(QByteArray(dataSize-data.size(), 0));
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
    for(int i=0; i<data.size(); i++)
    {
        QChar ch(data.constData()[i]);
        if(ch.isPrint())
            ascii+=ch.toAscii();
        else
            ascii+='.';
    }
    QString unicode;
    for(int i=0,j=0; i<data.size(); i+=2,j++)
    {
        QChar wch(((wchar_t*)data.constData())[j]);
        if(wch.isPrint())
            unicode+=wch;
        else
            unicode+='.';
    }
    ui->lineEditAscii->setText(ascii);
    ui->lineEditUnicode->setText(unicode);
}
