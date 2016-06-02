#include "HexValidator.h"
#include <QObject>

HexValidator::HexValidator(QObject* parent) : QValidator(parent)
{
}

HexValidator::~HexValidator()
{
}

void HexValidator::fixup(QString & input) const
{
    for(auto i : input)
    {
        if(!i.isDigit())
        {
            if(i >= QChar('a') && i <= QChar('f'))
                i = i.toUpper();
            else
                input.remove(i);
        }
    }
}

QValidator::State HexValidator::validate(QString & input, int & pos) const
{
    input = input.toUpper();
    for(int i = 0; i < input.length(); i++)
    {
        if(!(input.at(i).isDigit() || (input.at(i) >= QChar('A') && input.at(i) <= QChar('F'))))
            return State::Invalid;
    }
    return State::Acceptable;
}
