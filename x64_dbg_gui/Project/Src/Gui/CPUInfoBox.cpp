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

void CPUInfoBox::disasmSelectionChanged(int_t parVA)
{
    if(!DbgIsDebugging())
        return;
    QString info="";
    char section[10]="";
    bool bSection = DbgFunctions()->DbgSectionFromAddr(parVA, section);
    char label[MAX_LABEL_SIZE]="";
    if(DbgGetLabelAt(parVA, SEG_DEFAULT, label))
    {
        QString fullLabel="<"+QString(label)+">";
        char mod[MAX_MODULE_SIZE]="";
        if(DbgGetModuleAt(parVA, mod) && !QString(label).startsWith("JMP.&"))
            fullLabel="<"+QString(mod)+"."+QString(label)+">";
        if(bSection)
            info=QString(section)+":";
        info+=QString("%1").arg(parVA, sizeof(int_t) * 2, 16, QChar('0')).toUpper() + " " + fullLabel;
    }
    else if(bSection)
        info=QString(section)+":"+QString("%1").arg(parVA, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    setInfoLine(2, info);
    //setInfoLine(0, QString("%1").arg(parVA, sizeof(int_t) * 2, 16, QChar('0')).toUpper());
}
