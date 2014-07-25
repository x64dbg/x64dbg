#include "CPUInfoBox.h"

CPUInfoBox::CPUInfoBox(StdTable *parent) : StdTable(parent)
{
    enableMultiSelection(false);
    setShowHeader(false);
    addColumnAt(0, "", true);
    setRowCount(3);
    setCellContent(0, 0, "");
    setCellContent(1, 0, "");
    setCellContent(2, 0, "");
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    int height = getHeight();
    setMaximumHeight(height);
    setMinimumHeight(height);
    setCopyMenuOnly(true);
    connect(Bridge::getBridge(), SIGNAL(dbgStateChanged(DBGSTATE)), this, SLOT(dbgStateChanged(DBGSTATE)));
}

int CPUInfoBox::getHeight()
{
    return (getRowHeight() + 1) * 3;
}

void CPUInfoBox::setInfoLine(int line, QString text)
{
    if(line < 0 || line > 2)
        return;
    setCellContent(line, 0, text);
    reloadData();
}

void CPUInfoBox::clear()
{
    setInfoLine(0, "");
    setInfoLine(1, "");
    setInfoLine(2, "");
}

void CPUInfoBox::disasmSelectionChanged(int_t parVA)
{
    if(!DbgIsDebugging() || !parVA)
        return;
    clear();
    QString info;
    char mod[MAX_MODULE_SIZE]="";
    if(DbgFunctions()->ModNameFromAddr(parVA, mod, true))
    {
        int_t modbase=DbgFunctions()->ModBaseFromAddr(parVA);
        if(modbase)
            info=QString(mod)+"["+QString("%1").arg(parVA-modbase, 0, 16, QChar('0')).toUpper()+"] | ";
        else
            info=QString(mod)+" | ";
    }
    char section[10]="";
    if(DbgFunctions()->SectionFromAddr(parVA, section))
        info+="\"" + QString(section) + "\":";
    info+=QString("%1").arg(parVA, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    char label[MAX_LABEL_SIZE]="";
    if(DbgGetLabelAt(parVA, SEG_DEFAULT, label))
        info+=" <" + QString(label) + ">";
    setInfoLine(2, info);
}

void CPUInfoBox::dbgStateChanged(DBGSTATE state)
{
    if(state==stopped)
        clear();
}
