#pragma once

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
    explicit VirtualModDialog(QWidget* parent = nullptr);
    ~VirtualModDialog();
    bool getData(QString & modname, duint & base, duint & size);
    void setData(const QString & modname, duint base, duint size);

private:
    Ui::VirtualModDialog* ui;
};
