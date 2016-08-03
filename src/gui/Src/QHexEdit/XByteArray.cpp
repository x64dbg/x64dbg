#include "XByteArray.h"

XByteArray::XByteArray()
{
}

QByteArray & XByteArray::data()
{
    return _data;
}

void XByteArray::setData(const QByteArray & data)
{
    _data = data;
}

int XByteArray::size()
{
    return _data.size();
}

QByteArray & XByteArray::insert(int i, char ch)
{
    _data.insert(i, ch);
    return _data;
}

QByteArray & XByteArray::insert(int i, const QByteArray & ba)
{
    _data.insert(i, ba);
    return _data;
}

QByteArray & XByteArray::remove(int i, int len)
{
    _data.remove(i, len);
    return _data;
}

QByteArray & XByteArray::replace(int index, char ch)
{
    _data[index] = ch;
    return _data;
}

QByteArray & XByteArray::replace(int index, const QByteArray & ba)
{
    int len = ba.length();
    return replace(index, len, ba);
}

QByteArray & XByteArray::replace(int index, int length, const QByteArray & ba)
{
    int len;
    if((index + length) > _data.length())
        len = _data.length() - index;
    else
        len = length;
    _data.replace(index, len, ba.mid(0, len));
    return _data;
}
