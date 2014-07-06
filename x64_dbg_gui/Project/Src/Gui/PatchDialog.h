#ifndef PATCHDIALOG_H
#define PATCHDIALOG_H

#include <QDialog>
#include "Bridge.h"

namespace Ui {
    class PatchDialog;
}

class PatchDialog : public QDialog
{
    Q_OBJECT

    struct STATUSINFO
    {
        bool checked;
        int group;
    };

    typedef QPair<DBGPATCHINFO, STATUSINFO> PatchPair;
    typedef QList<PatchPair> PatchInfoList;
    typedef QMap<QString, PatchInfoList> PatchMap;

    static bool PatchInfoLess(const PatchPair & a, const PatchPair & b)
    {
        return a.first.addr < b.first.addr;
    }

public:
    explicit PatchDialog(QWidget *parent = 0);
    ~PatchDialog();

private:
    Ui::PatchDialog *ui;
    PatchMap* mPatches;
    bool mIsAdding;

    bool isPartOfPreviousGroup(PatchInfoList & patchList, int index);

private slots:
    void updatePatches();
    void on_listModules_itemSelectionChanged();
    void on_listPatches_itemChanged(QListWidgetItem *item);
    void on_btnSelectAll_clicked();
    void on_btnDeselectAll_clicked();
    void on_btnRestoreSelected_clicked();
    void on_listPatches_itemSelectionChanged();
};

#endif // PATCHDIALOG_H
