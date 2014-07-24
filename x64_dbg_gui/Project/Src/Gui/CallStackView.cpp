#include "CallStackView.h"
#include "Bridge.h"

CallStackView::CallStackView(StdTable* parent) : StdTable(parent)
{
    int charwidth = getCharWidth();

    addColumnAt(8+charwidth*sizeof(int_t)*2, "Address", true); //address in the stack
    addColumnAt(8+charwidth*sizeof(int_t)*2, "To", true); //return to
    addColumnAt(8+charwidth*sizeof(int_t)*2, "From", true); //return from
    addColumnAt(0, "Comment", true);

    connect(Bridge::getBridge(), SIGNAL(updateCallStack()), this, SLOT(updateCallStack()));

    setCopyMenuOnly(true);
}

void CallStackView::updateCallStack()
{
    DBGCALLSTACK callstack;
    memset(&callstack, 0, sizeof(DBGCALLSTACK));
    DbgFunctions()->GetCallStack(&callstack);
    setRowCount(callstack.total);
    for(int i=0; i<callstack.total; i++)
    {
        QString addrText=QString("%1").arg((uint_t)callstack.entries[i].addr, sizeof(uint_t)*2, 16, QChar('0')).toUpper();
        setCellContent(i, 0, addrText);
        addrText=QString("%1").arg((uint_t)callstack.entries[i].to, sizeof(uint_t)*2, 16, QChar('0')).toUpper();
        setCellContent(i, 1, addrText);
        addrText=QString("%1").arg((uint_t)callstack.entries[i].from, sizeof(uint_t)*2, 16, QChar('0')).toUpper();
        setCellContent(i, 2, addrText);
        setCellContent(i, 3, callstack.entries[i].comment);
    }
    if(callstack.total)
        BridgeFree(callstack.entries);
    reloadData();
}
