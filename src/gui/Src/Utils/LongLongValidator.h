#pragma once

#include <QValidator>

class LongLongValidator : public QValidator
{
    Q_OBJECT
public:
    enum DataType
    {
        SignedShort,
        UnsignedShort,
        SignedLong,
        UnsignedLong,
        SignedLongLong,
        UnsignedLongLong
    };

    explicit LongLongValidator(DataType t, QObject* parent = 0);
    ~LongLongValidator();

    void fixup(QString & input) const;
    State validate(QString & input, int & pos) const;
private:
    DataType dt;
};
