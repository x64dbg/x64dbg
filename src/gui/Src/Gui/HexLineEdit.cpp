#include <QTextCodec>
#include "Configuration.h"
#include "HexLineEdit.h"
#include "ui_HexLineEdit.h"
#include "Bridge.h"
#include <QKeyEvent>

HexLineEdit::HexLineEdit(QWidget* parent) :
    QLineEdit(parent),
    ui(new Ui::HexLineEdit)
{
    ui->setupUi(this);

    // setup data
    mData = QByteArray();
    mKeepSize = false;
    mOverwriteMode = false;
    mEncoding = QTextCodec::codecForName("System");

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
        QString newText = event->text();
        if(!newText.isEmpty() && newText.at(0).isPrint())
        {
            for(int i = 0; i < newText.size(); i++)
                del();
            QTextCodec::ConverterState converter(QTextCodec::IgnoreHeader);
            insert(mEncoding->fromUnicode(newText.constData(), newText.size(), &converter));
            event->ignore();
            return;
        }
    }

    QLineEdit::keyPressEvent(event);
}

void HexLineEdit::setData(const QByteArray & data)
{
    QString text;
    text = mEncoding->toUnicode(data);

    mData = data;
    setText(text);
}

QByteArray HexLineEdit::data()
{
    return mData;
}

/**
 * @brief HexLineEdit::setEncoding Set the encoding of the line edit.
 * @param encoding The codec for the line edit.
 * @remarks the parameter passed in will be managed by the widget. You must use a new codec.
 */
void HexLineEdit::setEncoding(QTextCodec* encoding)
{
    mEncoding = encoding;
}

/**
 * @brief HexLineEdit::encoding Get the encoding of the line edit.
 * @return the codec instance of the line edit.
 */
QTextCodec* HexLineEdit::encoding()
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
        QTextCodec::ConverterState converter(QTextCodec::IgnoreHeader);
        charSize = mEncoding->fromUnicode(QString("A").constData(), 1, &converter).size(); // "A\0"

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
    QTextCodec::ConverterState converter(QTextCodec::IgnoreHeader);
    return mEncoding->fromUnicode(text.constData(), text.size(), &converter);
}
