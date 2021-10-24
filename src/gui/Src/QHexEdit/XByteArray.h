#pragma once

#include <QByteArray>

class XByteArray
{
public:
    explicit XByteArray();

    QByteArray & data();
    void setData(const QByteArray & data);
    int size();

    QByteArray & insert(int i, char ch);
    QByteArray & insert(int i, const QByteArray & ba);

    QByteArray & remove(int pos, int len);

    QByteArray & replace(int index, char ch);
    QByteArray & replace(int index, const QByteArray & ba);
    QByteArray & replace(int index, int length, const QByteArray & ba);

private:
    QByteArray _data; //raw byte array
};
