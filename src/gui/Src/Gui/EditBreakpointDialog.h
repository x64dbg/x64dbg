#pragma once

#include <QDialog>
#include "Bridge.h"
#include "Breakpoints.h"

namespace Ui
{
    class EditBreakpointDialog;
}

class EditBreakpointDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditBreakpointDialog(QWidget* parent, const Breakpoints::Data & bp);
    ~EditBreakpointDialog();
    const Breakpoints::Data & getBp()
    {
        return mBp;
    }

private slots:
    void on_editLogText_textEdited(const QString & arg1);
    void on_buttonLogFile_clicked();
    void acceptedSlot();

private:
    Ui::EditBreakpointDialog* ui;
    Breakpoints::Data mBp;
    QString mLogFile;

    void loadFromBp();
};
