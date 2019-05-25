#ifndef EDITBREAKPOINTSDIALOG_H
#define EDITBREAKPOINTSDIALOG_H

#include <QDialog>
#include "Bridge.h"

namespace Ui
{
    class EditBreakpointsDialog;
}

class EditBreakpointsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditBreakpointsDialog(QWidget* parent, QString winTitle);
    ~EditBreakpointsDialog();
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
    Ui::EditBreakpointsDialog* ui;
    BRIDGEBP mBp;

    void loadFromBp();
};

#endif // EDITBREAKPOINTSDIALOG_H
