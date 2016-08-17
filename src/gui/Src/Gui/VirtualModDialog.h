#ifndef VIRTUALMODDIALOG_H
#define VIRTUALMODDIALOG_H

#include <QDialog>
#include "Imports.h"

namespace Ui
{
    class VirtualModDialog;
}

class VirtualModDialog : public QDialog
{
    Q_OBJECT

public:
    explicit VirtualModDialog(QWidget* parent = 0);
    ~VirtualModDialog();
    bool getData(QString & modname, duint & base, duint & size);
    void setData(const QString & modname, duint base, duint size);

private:
    Ui::VirtualModDialog* ui;
};

#endif // VIRTUALMODDIALOG_H
