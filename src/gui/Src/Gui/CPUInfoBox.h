#ifndef INFOBOX_H
#define INFOBOX_H

#include "StdTable.h"

class WordEditDialog;
class XrefBrowseDialog;

class CPUInfoBox : public StdTable
{
    Q_OBJECT
public:
    explicit CPUInfoBox(StdTable* parent = 0);
    int getHeight();
    void addFollowMenuItem(QMenu* menu, QString name, duint value);
    void setupFollowMenu(QMenu* menu, duint wVA);
    void addModifyValueMenuItem(QMenu* menu, QString name, duint value);
    void setupModifyValueMenu(QMenu* menu, duint wVA);
    void addWatchMenuItem(QMenu* menu, QString name, duint value);
    void setupWatchMenu(QMenu* menu, duint wVA);
    int followInDump(dsint wVA);

public slots:
    void disasmSelectionChanged(dsint parVA);
    void dbgStateChanged(DBGSTATE state);
    void contextMenuSlot(QPoint pos);
    void followActionSlot();
    void modifySlot();
    void findXReferencesSlot();
    void copyAddress();
    void copyRva();
    void copyOffset();
    void doubleClickedSlot();
    void addInfoLine(const QString & infoLine);

private:
    dsint curAddr;
    dsint curRva;
    dsint curOffset;
    void setInfoLine(int line, QString text);
    QString getInfoLine(int line);
    void clear();
    void setupContextMenu();

    XrefBrowseDialog* mXrefDlg = nullptr;

    QAction* mCopyAddressAction;
    QAction* mCopyRvaAction;
    QAction* mCopyOffsetAction;
};

#endif // INFOBOX_H
