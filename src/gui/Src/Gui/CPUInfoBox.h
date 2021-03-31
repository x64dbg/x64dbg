#ifndef INFOBOX_H
#define INFOBOX_H

#include "Imports.h"
#include "StdTable.h"

class WordEditDialog;
class XrefBrowseDialog;
class QBeaEngine;

class CPUInfoBox : public StdTable
{
    Q_OBJECT
public:
    explicit CPUInfoBox(QWidget* parent = 0);
    ~CPUInfoBox();
    int getHeight();
    void addFollowMenuItem(QMenu* menu, QString name, duint value);
    void setupFollowMenu(QMenu* menu, duint wVA);
    void addModifyValueMenuItem(QMenu* menu, QString name, duint value);
    void setupModifyValueMenu(QMenu* menu, duint wVA);
    void addWatchMenuItem(QMenu* menu, QString name, duint value);
    void setupWatchMenu(QMenu* menu, duint wVA);
    int followInDump(dsint wVA);

    static QString formatSSEOperand(const QByteArray & data, uint8_t vectorType);

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
    void setupShortcuts();
    XrefBrowseDialog* mXrefDlg = nullptr;
    QBeaEngine* mDisasm;

    QAction* mCopyAddressAction;
    QAction* mCopyRvaAction;
    QAction* mCopyOffsetAction;
    QAction* mCopyLineAction;
};

#endif // INFOBOX_H
