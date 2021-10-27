#pragma once

#include "StdTable.h"

class CPUMultiDump;

class LocalVarsView : public StdTable
{
    Q_OBJECT
public:
    LocalVarsView(CPUMultiDump* parent);

public slots:
    void contextMenuSlot(const QPoint & pos);
    void mousePressEvent(QMouseEvent* event);

    void followDumpSlot();
    void followStackSlot();
    void followMemMapSlot();
    void followWordInDumpSlot();
    void followWordInStackSlot();
    void baseChangedSlot();
    void renameSlot();

    void updateSlot();
    void configUpdatedSlot();
    void editSlot();

private:
    duint currentFunc;
    void setupContextMenu();
    MenuBuilder* mMenu;

    bool HexPrefixValues;
    bool MemorySpaces;
#ifdef _WIN64
    QAction* baseRegisters[16];
#else //x86
    QAction* baseRegisters[8];
#endif //_WIN64
};
