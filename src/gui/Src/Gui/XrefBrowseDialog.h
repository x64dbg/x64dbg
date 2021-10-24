#pragma once

#include "Bridge.h"
#include "ActionHelpers.h"
#include <QDialog>
#include <QListWidgetItem>

namespace Ui
{
    class XrefBrowseDialog;
}

class XrefBrowseDialog : public QDialog, public ActionHelper<XrefBrowseDialog>
{
    Q_OBJECT

public:
    explicit XrefBrowseDialog(QWidget* parent);
    ~XrefBrowseDialog();
    using GotoFunction = std::function<void(duint)>;
    void setup(duint address, GotoFunction gotoFunction);

private slots:
    void on_listWidget_itemDoubleClicked(QListWidgetItem* item);
    void on_listWidget_itemSelectionChanged();
    void on_listWidget_currentRowChanged(int currentRow);
    void on_XrefBrowseDialog_rejected();
    void on_listWidget_itemClicked(QListWidgetItem* item);
    void on_listWidget_customContextMenuRequested(const QPoint & pos);

    void onDebuggerClose(DBGSTATE state);
    void memoryAccessSingleshootSlot();
    void memoryAccessRestoreSlot();
    void memoryWriteSingleshootSlot();
    void memoryWriteRestoreSlot();
    void memoryRemoveSlot();
    void hardwareAccess1Slot();
    void hardwareAccess2Slot();
    void hardwareAccess4Slot();
    void hardwareAccess8Slot();
    void hardwareWrite1Slot();
    void hardwareWrite2Slot();
    void hardwareWrite4Slot();
    void hardwareWrite8Slot();
    void hardwareRemoveSlot();
    void breakpointSlot();
    void copyThisSlot();
    void copyAllSlot();
    void breakpointAllSlot();

private:
    Ui::XrefBrowseDialog* ui;

    void changeAddress(duint address);
    void setupContextMenu();
    QString GetFunctionSymbol(duint addr);

    XREF_INFO mXrefInfo;
    duint mAddress;
    int mPrevSelectionSize;
    MenuBuilder* mMenu;
    GotoFunction mGotoFunction;
};
