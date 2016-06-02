#include "LongLongValidator.h"

LongLongValidator::LongLongValidator(DataType t, QObject* parent) : QValidator(parent), dt(t)
{
}

LongLongValidator::~LongLongValidator()
{
}

void LongLongValidator::fixup(QString & input) const
{
    Q_UNUSED(input);
}

QValidator::State LongLongValidator::validate(QString & input, int & pos) const
{
    Q_UNUSED(pos);
    bool ok = false;
    if(input.isEmpty()) return State::Acceptable;
    switch(dt)
    {
    case SignedShort:
        input.toShort(&ok);
        if(!ok)
        {
            if(input == QString('-'))
                return State::Acceptable;
            if(input.toULongLong(&ok) > 32767)
            {
                if(ok)
                {
                    input = "32767";
                    return State::Acceptable;
                }
            }
            else if(input.toLongLong(&ok) < -32768)
            {
                if(ok)
                {
                    input = "-32768";
                    return State::Acceptable;
                }
            }
            return State::Invalid;
        }
        return State::Acceptable;
    case UnsignedShort:
        input.toShort(&ok);
        if(!ok)
        {
            if(input.toULongLong(&ok) > 65535)
            {
                if(ok)
                {
                    input = "65535";
                    return State::Acceptable;
                }
            }
            return State::Invalid;
        }
        return State::Acceptable;
    case SignedLong:
        input.toLong(&ok);
        if(!ok)
        {
            if(input == QString('-'))
                return State::Acceptable;
            if(input.toULongLong(&ok) > 2147483647LL)
            {
                if(ok)
                {
                    input = "2147483647";
                    return State::Acceptable;
                }
            }
            else if(input.toLongLong(&ok) < -2147483648LL)
            {
                if(ok)
                {
                    input = "-2147483648";
                    return State::Acceptable;
                }
            }
            return State::Invalid;
        }
        return State::Acceptable;
    case UnsignedLong:
        input.toULong(&ok);
        if(!ok)
        {
            if(input.toULongLong(&ok) > 4294967295)
                if(ok)
                {
                    input = "4294967295";
                    return State::Acceptable;
                }
            return State::Invalid;
        }
        return State::Acceptable;
    case SignedLongLong:
        input.toLongLong(&ok);
        if(!ok)
            return input == QChar('-') ? State::Acceptable : State::Invalid;
        return State::Acceptable;
    case UnsignedLongLong:
        input.toULongLong(&ok);
        return ok ? State::Acceptable : State::Invalid;
    }
    return State::Invalid;
}
