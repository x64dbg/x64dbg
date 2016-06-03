#include "HexValidator.h"
#include <QObject>

HexValidator::HexValidator(QObject* parent) : QValidator(parent)
{
}

HexValidator::~HexValidator()
{
}

static bool isXDigit(const QChar & c)
{
    return c.isDigit() || (c.toUpper() >= 'A' && c.toUpper() <= 'F');
}

void HexValidator::fixup(QString & input) const
{
    for(auto i : input)
    {
        if(!isXDigit(i))
            input.remove(i);
    }
}

QValidator::State HexValidator::validate(QString & input, int & pos) const
{
    Q_UNUSED(pos);
    for(int i = 0; i < input.length(); i++)
    {
        if(!isXDigit(input[i]))
            return State::Invalid;
    }
    return State::Acceptable;
}
