#pragma once

#include <QValidator>

class HexValidator : public QValidator
{
    Q_OBJECT
public:
    explicit HexValidator(QObject* parent = nullptr);
    ~HexValidator();

    void fixup(QString & input) const;
    State validate(QString & input, int & pos) const;
};
