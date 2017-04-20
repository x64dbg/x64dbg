#ifndef LOCALVARSVIEW_H
#define LOCALVARSVIEW_H
#include "StdTable.h"

class CPUMultiDump;

class LocalVarsView : public StdTable
{
    Q_OBJECT
public:
    LocalVarsView(CPUMultiDump* parent);

public slots:
    void contextMenuSlot(const QPoint & pos);
    void updateSlot();
    void baseChangedSlot();
    void renameSlot();
    void editSlot();

private:
    duint currentFunc;
    void setupContextMenu();
    MenuBuilder* mMenu;
#ifdef _WIN64
    QAction* baseRegisters[16];
#else //x86
    QAction* baseRegisters[8];
#endif //_WIN64
};

#endif //LOCALVARSVIEW_H
