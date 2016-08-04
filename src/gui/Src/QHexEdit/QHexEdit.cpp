#include <QtGui>

#include "QHexEdit.h"
#include "QHexEditPrivate.h"

QHexEdit::QHexEdit(QWidget* parent) : QScrollArea(parent)
{
    qHexEdit_p = new QHexEditPrivate(this);
    setWidget(qHexEdit_p);
    setWidgetResizable(true);

    connect(qHexEdit_p, SIGNAL(currentAddressChanged(int)), this, SIGNAL(currentAddressChanged(int)));
    connect(qHexEdit_p, SIGNAL(currentSizeChanged(int)), this, SIGNAL(currentSizeChanged(int)));
    connect(qHexEdit_p, SIGNAL(dataChanged()), this, SIGNAL(dataChanged()));
    connect(qHexEdit_p, SIGNAL(dataEdited()), this, SIGNAL(dataEdited()));
    connect(qHexEdit_p, SIGNAL(overwriteModeChanged(bool)), this, SIGNAL(overwriteModeChanged(bool)));

    setFocusPolicy(Qt::NoFocus);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
}

void QHexEdit::setEditFont(const QFont & font)
{
    qHexEdit_p->setFont(font);
}

void QHexEdit::insert(int i, const QByteArray & ba, const QByteArray & mask)
{
    qHexEdit_p->insert(i, ba, mask);
}

void QHexEdit::insert(int i, char ch, char mask)
{
    qHexEdit_p->insert(i, ch, mask);
}

void QHexEdit::remove(int pos, int len)
{
    qHexEdit_p->remove(pos, len);
}

void QHexEdit::replace(int pos, int len, const QByteArray & after, const QByteArray & mask)
{
    qHexEdit_p->replace(pos, len, after, mask);
}

void QHexEdit::fill(int index, const QString & pattern)
{
    QString convert;
    for(int i = 0; i < pattern.length(); i++)
    {
        QChar ch = pattern[i].toLower();
        if((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (wildcardEnabled() && ch == '?'))
            convert += ch;
    }
    if(!convert.length())
        return;
    if(convert.length() % 2) //odd length
        convert += "0";
    QByteArray data(convert.length(), 0);
    QByteArray mask(data.length(), 0);
    for(int i = 0; i < convert.length(); i++)
    {
        if(convert[i] == '?')
        {
            data[i] = '0';
            mask[i] = '1';
        }
        else
        {
            data[i] = convert[i].toLatin1();
            mask[i] = '0';
        }
    }
    qHexEdit_p->fill(index, QByteArray().fromHex(data), QByteArray().fromHex(mask));
}

void QHexEdit::redo()
{
    qHexEdit_p->redo();
}

void QHexEdit::undo()
{
    qHexEdit_p->undo();
}

void QHexEdit::setCursorPosition(int cursorPos)
{
    // cursorPos in QHexEditPrivate is the position of the textcoursor without
    // blanks, means bytePos*2
    qHexEdit_p->setCursorPos(cursorPos * 2);
}

int QHexEdit::cursorPosition()
{
    return qHexEdit_p->cursorPos() / 2;
}

void QHexEdit::setData(const QByteArray & data, const QByteArray & mask)
{
    qHexEdit_p->setData(data, mask);
}

void QHexEdit::setData(const QByteArray & data)
{
    qHexEdit_p->setData(data, QByteArray(data.size(), 0));
}

void QHexEdit::setData(const QString & pattern)
{
    QString convert;
    for(int i = 0; i < pattern.length(); i++)
    {
        QChar ch = pattern[i].toLower();
        if((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (wildcardEnabled() && ch == '?'))
            convert += ch;
    }
    if(convert.length() % 2) //odd length
        convert += "0";
    QByteArray data(convert.length(), 0);
    QByteArray mask(data.length(), 0);
    for(int i = 0; i < convert.length(); i++)
    {
        if(convert[i] == '?')
        {
            data[i] = '0';
            mask[i] = '1';
        }
        else
        {
            data[i] = convert[i].toLatin1();
            mask[i] = '0';
        }
    }
    qHexEdit_p->setData(QByteArray().fromHex(data), QByteArray().fromHex(mask));
}

QByteArray QHexEdit::applyMaskedData(const QByteArray & data)
{
    QByteArray ret = data.toHex();
    QByteArray _data = this->data().toHex();
    QByteArray _mask = this->mask().toHex();
    if(ret.size() != _data.size())
        ret.resize(_data.size());
    for(int i = 0; i < _data.size(); i++)
    {
        if(_mask[i] != '1')
            ret[i] = _data[i];
    }
    return QByteArray().fromHex(ret);
}

QByteArray QHexEdit::data()
{
    return qHexEdit_p->data();
}

QByteArray QHexEdit::mask()
{
    return qHexEdit_p->mask();
}

QString QHexEdit::pattern(bool space)
{
    QString result;
    for(int i = 0; i < this->data().size(); i++)
    {
        QString byte = this->data().mid(i, 1).toHex();
        QString mask = this->mask().mid(i, 1).toHex();
        if(mask[0] == '1')
            result += "?";
        else
            result += byte[0];
        if(mask[1] == '1')
            result += "?";
        else
            result += byte[1];
        if(space)
            result += " ";
    }
    return result.toUpper().trimmed();
}

void QHexEdit::setOverwriteMode(bool overwriteMode)
{
    qHexEdit_p->setOverwriteMode(overwriteMode);
}

bool QHexEdit::overwriteMode()
{
    return qHexEdit_p->overwriteMode();
}

void QHexEdit::setWildcardEnabled(bool enabled)
{
    qHexEdit_p->setWildcardEnabled(enabled);
}

bool QHexEdit::wildcardEnabled()
{
    return qHexEdit_p->wildcardEnabled();
}

void QHexEdit::setKeepSize(bool enabled)
{
    qHexEdit_p->setKeepSize(enabled);
}

bool QHexEdit::keepSize()
{
    return qHexEdit_p->keepSize();
}

void QHexEdit::setTextColor(QColor color)
{
    qHexEdit_p->setTextColor(color);
}

void QHexEdit::setHorizontalSpacing(int x)
{
    qHexEdit_p->setHorizontalSpacing(x);
}

int QHexEdit::horizontalSpacing()
{
    return qHexEdit_p->horizontalSpacing();
}

QColor QHexEdit::textColor()
{
    return qHexEdit_p->textColor();
}

void QHexEdit::setWildcardColor(QColor color)
{
    qHexEdit_p->setWildcardColor(color);
}

QColor QHexEdit::wildcardColor()
{
    return qHexEdit_p->wildcardColor();
}

void QHexEdit::setBackgroundColor(QColor color)
{
    qHexEdit_p->setBackgroundColor(color);
}

QColor QHexEdit::backgroundColor()
{
    return qHexEdit_p->backgroundColor();
}

void QHexEdit::setSelectionColor(QColor color)
{
    qHexEdit_p->setSelectionColor(color);
}

QColor QHexEdit::selectionColor()
{
    return qHexEdit_p->selectionColor();
}
