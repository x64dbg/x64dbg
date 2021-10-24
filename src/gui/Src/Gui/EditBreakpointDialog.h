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
    void on_editName_textEdited(const QString & arg1);
    void on_editBreakCondition_textEdited(const QString & arg1);
    void on_editLogText_textEdited(const QString & arg1);
    void on_editLogCondition_textEdited(const QString & arg1);
    void on_editCommandText_textEdited(const QString & arg1);
    void on_editCommandCondition_textEdited(const QString & arg1);
    void on_checkBoxFastResume_toggled(bool checked);
    void on_spinHitCount_valueChanged(int arg1);
    void on_checkBoxSilent_toggled(bool checked);
    void on_checkBoxSingleshoot_toggled(bool checked);

private:
    Ui::EditBreakpointDialog* ui;
    BRIDGEBP mBp;

    void loadFromBp();
};
