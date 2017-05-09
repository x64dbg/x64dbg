#ifndef INFOBOX_H
#define INFOBOX_H

#include "StdTable.h"

class WordEditDialog;

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
    void findReferencesSlot();
    void copyAddress();
    void copyRva();
    void copyOffset();
    void doubleClickedSlot();
    void addInfoLine(const QString & infoLine);

signals:
    void displayReferencesWidget();

private:
    dsint curAddr;
    dsint curRva;
    dsint curOffset;
    QString getSymbolicName(dsint addr);
    void setInfoLine(int line, QString text);
    QString getInfoLine(int line);
    void clear();
    void setupContextMenu();

    QAction* mCopyAddressAction;
    QAction* mCopyRvaAction;
    QAction* mCopyOffsetAction;
};

#endif // INFOBOX_H
