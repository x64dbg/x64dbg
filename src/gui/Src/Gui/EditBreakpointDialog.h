#pragma once

#include <QDialog>
#include "Bridge.h"

namespace Ui
{
    class EditBreakpointDialog;
}

class EditBreakpointDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditBreakpointDialog(QWidget* parent, const BRIDGEBP & bp);
    ~EditBreakpointDialog();
    const BRIDGEBP & getBp()
    {
        return mBp;
    }

private slots:
    void on_editLogText_textEdited(const QString & arg1);
    void acceptedSlot();

private:
    Ui::EditBreakpointDialog* ui;
    BRIDGEBP mBp;

    void loadFromBp();
};
