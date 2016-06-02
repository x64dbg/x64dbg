#ifndef HEXVALIDATOR_H
#define HEXVALIDATOR_H
#include <QValidator>

class HexValidator : public QValidator
{
    Q_OBJECT
public:
    explicit HexValidator(QObject* parent = 0);
    ~HexValidator();

    void fixup(QString & input) const;
    State validate(QString & input, int & pos) const;
};

#endif // HEXVALIDATOR_H
