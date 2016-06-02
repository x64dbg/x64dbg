#ifndef XREFBROWSEDIALOG_H
#define XREFBROWSEDIALOG_H

#include <QDialog>
#include <capstone_wrapper.h>
#include "XrefBrowseDialog.h"
#include "Bridge.h"


namespace Ui
{
    class XrefBrowseDialog;
}

class XrefBrowseDialog : public QDialog
{
    Q_OBJECT

public:
    explicit XrefBrowseDialog(QWidget* parent, duint address);
    ~XrefBrowseDialog();

public slots:

    void on_buttonCancel_clicked();
    void on_listview_clicked(int row);

private:
    Ui::XrefBrowseDialog* ui;
    XREF_INFO mXrefInfo;
    Capstone mCapstone;
    duint mAddress;
};

#endif // XREFBROWSEDIALOG_H
