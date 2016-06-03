#ifndef XREFBROWSEDIALOG_H
#define XREFBROWSEDIALOG_H

#include "Bridge.h"
#include <QDialog>
#include <QListWidgetItem>

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

private slots:
    void on_listWidget_itemDoubleClicked(QListWidgetItem* item);
    void on_listWidget_itemSelectionChanged();
    void on_listWidget_currentRowChanged(int currentRow);
    void on_XrefBrowseDialog_rejected();

private:
    Ui::XrefBrowseDialog* ui;
    XREF_INFO mXrefInfo;
    duint mAddress;
    int mPrevSelectionSize;
    void changeAddress(duint address);
};

#endif // XREFBROWSEDIALOG_H
