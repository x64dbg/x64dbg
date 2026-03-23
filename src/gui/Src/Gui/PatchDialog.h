#pragma once

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

    struct PatchPair
    {
        DBGPATCHINFO patch;
        STATUSINFO status;

        PatchPair(const DBGPATCHINFO & patch, const STATUSINFO & status)
        {
            this->patch = patch;
            this->status = status;
        }
    };

    enum class ActiveCommentType
    {
        None = 0,
        Module,
        Address,
        Stale
    };

    //typedef QPair<DBGPATCHINFO, STATUSINFO> PatchPair;
    typedef QList<PatchPair> PatchInfoList;
    typedef QMap<QString, PatchInfoList> PatchMap;
    typedef QMap<QString, QString> PatchModuleCommentsMap;
    typedef QMap<QString, QMap<QString, QString>> PatchAddressCommentsMap;

    static bool PatchInfoLess(const PatchPair & a, const PatchPair & b)
    {
        return a.patch.addr < b.patch.addr;
    }

public:
    explicit PatchDialog(QWidget* parent = nullptr);
    ~PatchDialog();

private:
    Ui::PatchDialog* ui;
    PatchMap mPatches;
    PatchModuleCommentsMap mModulePatchComments;
    PatchAddressCommentsMap mAddressPatchComments;
    ActiveCommentType mActiveCommentType;
    PatchDialogGroupSelector* mGroupSelector;
    bool mIsWorking;

    bool isPartOfPreviousGroup(const PatchInfoList & patchList, int index);
    bool isGroupEnabled(const PatchInfoList & patchList, int group);
    bool hasPreviousGroup(const PatchInfoList & patchList, int group);
    bool hasNextGroup(const PatchInfoList & patchList, int group);
    dsint getGroupAddress(const PatchInfoList & patchList, int group);

    void saveAs1337(const QString & filename);
    //void saveAsC(const QString & filename);

    bool containsRelocatedBytes();
    bool containsRelocatedBytes(const PatchInfoList & patchList);
    bool showRelocatedBytesWarning();

    void showEvent(QShowEvent* event);
    void closeEvent(QCloseEvent* event);

    QListWidgetItem* getSelectedOrFirst(QListWidget* items);
    QString getCommentKeyForPatchInfo(const DBGPATCHINFO & patchInfo);
    bool getPatchInfoForModuleFromUi(QListWidgetItem* item, DBGPATCHINFO & patchInfo);
    bool getPatchInfoForAddressFromUi(QListWidgetItem* item, DBGPATCHINFO & patchInfo);
    void loadCommentForModule(const DBGPATCHINFO & patchInfo);
    void loadCommentForAddress(const DBGPATCHINFO & patchInfo);
    void saveCommentForModule(const DBGPATCHINFO & patchInfo, const QString & comment);
    void saveCommentForAddress(const DBGPATCHINFO & patchInfo, const QString & comment);
    void syncComment(QListWidgetItem* current, QListWidgetItem* previous, ActiveCommentType previousCommentType, ActiveCommentType incomingCommentType);
    void syncComment(QListWidgetItem* item, ActiveCommentType previousCommentType, ActiveCommentType incomingCommentType);
    bool isCommentForModule(ActiveCommentType type);
    bool isCommentForAddress(ActiveCommentType type);
    bool isCommentStale(ActiveCommentType type);
    bool isCommentUninitialized(ActiveCommentType type);
    bool isEligibleForComment(ActiveCommentType type);
private slots:
    void dbgStateChanged(DBGSTATE state);
    void updatePatches();
    void groupToggle();
    void groupPrevious();
    void groupNext();
    void on_listModules_itemSelectionChanged();
    void on_listModules_currentItemChanged(QListWidgetItem* current, QListWidgetItem* previous);
    void on_listModules_itemClicked(QListWidgetItem* item);
    void on_listPatches_currentItemChanged(QListWidgetItem* current, QListWidgetItem* previous);
    void on_listPatches_itemClicked(QListWidgetItem* item);
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
