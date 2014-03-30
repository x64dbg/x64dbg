#include "InfoBox.h"

InfoBox::InfoBox(StdTable *parent) : StdTable(parent)
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
}

int InfoBox::getHeight()
{
    return (getRowHeight() + 1) * 3;
}

void InfoBox::setInfoLine(int line, QString text)
{
    if(line < 0 || line > 2)
        return;
    setCellContent(line, 0, text);
    reloadData();
}

void InfoBox::disasmSelectionChanged(int_t parVA)
{
    //setInfoLine(0, QString("%1").arg(parVA, sizeof(int_t) * 2, 16, QChar('0')).toUpper());
}
