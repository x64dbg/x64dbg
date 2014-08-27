#ifndef PATCHDIALOG_H
#define PATCHDIALOG_H

#include <QDialog>
#include <QListWidgetItem>
#include "PatchDialogGroupSelector.h"
#include "Bridge.h"

namespace Ui
{
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
    explicit PatchDialog(QWidget* parent = 0);
    ~PatchDialog();

private:
    Ui::PatchDialog* ui;
    PatchMap* mPatches;
    PatchDialogGroupSelector* mGroupSelector;
    bool mIsWorking;

    bool isPartOfPreviousGroup(const PatchInfoList & patchList, int index);
    bool isGroupEnabled(const PatchInfoList & patchList, int group);
    bool hasPreviousGroup(const PatchInfoList & patchList, int group);
    bool hasNextGroup(const PatchInfoList & patchList, int group);
    int_t getGroupAddress(const PatchInfoList & patchList, int group);

private slots:
    void dbgStateChanged(DBGSTATE state);
    void updatePatches();
    void groupToggle();
    void groupPrevious();
    void groupNext();
    void on_listModules_itemSelectionChanged();
    void on_listPatches_itemChanged(QListWidgetItem* item);
    void on_btnSelectAll_clicked();
    void on_btnDeselectAll_clicked();
    void on_btnRestoreSelected_clicked();
    void on_listPatches_itemSelectionChanged();
    void on_btnPickGroups_clicked();
    void on_btnPatchFile_clicked();
    void on_btnImport_clicked();
    void on_btnExport_clicked();
};

#endif // PATCHDIALOG_H
