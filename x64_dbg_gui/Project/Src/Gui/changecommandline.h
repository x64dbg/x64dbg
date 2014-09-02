#ifndef CHANGECOMMANDLINE_H
#define CHANGECOMMANDLINE_H

#include <QDialog>
#include "NewTypes.h"

namespace Ui
{
class ChangeCommandline;
}

class ChangeCommandline : public QDialog
{
    Q_OBJECT

public:
    explicit ChangeCommandline(QWidget* parent = 0);
    ~ChangeCommandline();

private slots:
    void on_buttonBox_accepted();

private:
    Ui::ChangeCommandline* ui;
};

#endif // CHANGECOMMANDLINE_H
