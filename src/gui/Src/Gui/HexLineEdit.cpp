#include <QTextCodec>

#include "HexLineEdit.h"
#include "ui_HexLineEdit.h"
#include "Bridge.h"

HexLineEdit::HexLineEdit(QWidget* parent) :
    QLineEdit(parent),
    ui(new Ui::HexLineEdit)
{
    ui->setupUi(this);

    // setup data
    mData = QByteArray();
    mEncoding = Encoding::Ascii;
    mKeepSize = false;
    mOverwriteMode = false;

    //setup text fields
    QFont font("Monospace", 8, QFont::Normal, false);
    font.setFixedPitch(true);
    font.setStyleHint(QFont::Monospace);
    setFont(font);

    connect(this, SIGNAL(textEdited(const QString &)), this, SLOT(updateData(const QString &)));
}

HexLineEdit::~HexLineEdit()
{
    delete ui;
}

void HexLineEdit::keyPressEvent(QKeyEvent* event)
{
    // Switch between insert/overwrite mode
    if(event->key() == Qt::Key_Insert && (event->modifiers() == Qt::NoModifier))
    {
        mOverwriteMode = !mOverwriteMode;
        event->ignore();
        return;
    }

    if(mOverwriteMode)
    {
        if(!event->text().isEmpty() && event->text().at(0).isPrint())
        {
            QString keyText;
            switch(mEncoding)
            {
            case Encoding::Ascii:
                keyText  = event->text().toLatin1();
                break;
            case Encoding::Unicode:
                keyText  = event->text();
                break;
            }

            del();
            insert(keyText);
            event->ignore();
            return;
        }
    }

    QLineEdit::keyPressEvent(event);
}

void HexLineEdit::setData(const QByteArray & data)
{
    QString text;
    switch(mEncoding)
    {
    case Encoding::Ascii:
        for(int i = 0; i < data.size(); i++)
        {
            QChar ch(data.constData()[i]);
            if(ch.isPrint())
                text += ch.toLatin1();
            else
                text += '.';
        }
        break;

    case Encoding::Unicode:
        for(int i = 0, j = 0; i < data.size(); i += sizeof(wchar_t), j++)
        {
            QChar wch(((wchar_t*)data.constData())[j]);
            if(wch.isPrint())
                text += wch;
            else
                text += '.';
        }
        break;
    }

    mData = toEncodedData(text);
    setText(text);
}

QByteArray HexLineEdit::data()
{
    return mData;
}

void HexLineEdit::setEncoding(const HexLineEdit::Encoding encoding)
{
    mEncoding = encoding;
}

HexLineEdit::Encoding HexLineEdit::encoding()
{
    return mEncoding;
}

void HexLineEdit::setKeepSize(const bool enabled)
{
    mKeepSize = enabled;
    if(enabled)
    {
        int dataSize = mData.size();
        int charSize;
        switch(mEncoding)
        {
        case Encoding::Ascii:
            charSize = sizeof(char);
            break;

        case Encoding::Unicode:
            charSize = sizeof(wchar_t);
            break;
        }

        setMaxLength((dataSize / charSize) + (dataSize % charSize));
    }
    else
    {
        setMaxLength(32767);
    }
}

bool HexLineEdit::keepSize()
{
    return mKeepSize;
}

void HexLineEdit::setOverwriteMode(bool overwriteMode)
{
    mOverwriteMode = overwriteMode;
}

bool HexLineEdit::overwriteMode()
{
    return mOverwriteMode;
}

void HexLineEdit::updateData(const QString & arg1)
{
    Q_UNUSED(arg1);

    mData = toEncodedData(text());
    emit dataEdited();
}

QByteArray HexLineEdit::toEncodedData(const QString & text)
{
    QByteArray data;
    switch(mEncoding)
    {
    case Encoding::Ascii:
        for(int i = 0; i < text.length(); i++)
            data.append(text[i].toLatin1());
        break;

    case Encoding::Unicode:
        data =  QTextCodec::codecForName("UTF-16")->makeEncoder(QTextCodec::IgnoreHeader)->fromUnicode(text);
        break;
    }

    return data;
}
