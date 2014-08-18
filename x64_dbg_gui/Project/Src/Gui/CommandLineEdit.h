#ifndef COMMANDLINEEDIT_H
#define COMMANDLINEEDIT_H

#include "HistoryLineEdit.h"
#include <QCompleter>

class CommandLineEdit : public HistoryLineEdit
{
    Q_OBJECT
public:
    explicit CommandLineEdit(QWidget* parent = 0);

public slots:
    void autoCompleteAddCmd(const QString cmd);
    void autoCompleteDelCmd(const QString cmd);
    void autoCompleteClearAll();

private:
    QCompleter* mCompleter;
};

#endif // COMMANDLINEEDIT_H
