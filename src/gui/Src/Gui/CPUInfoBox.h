#pragma once

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
    void setupFollowMenu(QMenu* menu, duint va);
    void addModifyValueMenuItem(QMenu* menu, QString name, duint value);
    void setupModifyValueMenu(QMenu* menu, duint va);
    void addWatchMenuItem(QMenu* menu, QString name, duint value);
    void setupWatchMenu(QMenu* menu, duint va);
    int followInDump(duint va);

    static QString formatSSEOperand(const QByteArray & data, unsigned char vectorType);

public slots:
    void disasmSelectionChanged(duint parVA);
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
    duint mCurAddr = 0;
    duint mCurRva = 0;
    duint mCurOffset = 0;
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
