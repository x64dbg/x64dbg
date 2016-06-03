#ifndef XREFBROWSEDIALOG_H
#define XREFBROWSEDIALOG_H

#include <QDialog>
#include <QListWidgetItem>
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
    void on_currentRow_changed(int);
    void on_item_DoubleClicked(QListWidgetItem*);
    void on_selection_changed();

private:
    Ui::XrefBrowseDialog* ui;
    XREF_INFO mXrefInfo;
    duint mAddress;
    int mPrevSelectionSize;
    void changeAddress(duint address);
};

#endif // XREFBROWSEDIALOG_H
