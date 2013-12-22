#include "InfoBox.h"

InfoBox::InfoBox(StdTable *parent) : StdTable(parent)
{
    enableMultiSelection(false);
    setShowHeader(false);
    addColumnAt(0, "", true);
    setRowCount(3);
    setCellContent(0, 0, "Info Line 1");
    setCellContent(1, 0, "Info Line 2");
    setCellContent(2, 0, "Info Line 3");
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    int height = getHeight();
    setMaximumHeight(height);
    setMinimumHeight(height);
    connect(Bridge::getBridge(), SIGNAL(setInfoLine(int, QString)), this, SLOT(setInfoLineSlot(int, QString)));
}

int InfoBox::getHeight()
{
    return (getRowHeight() + 1) * 3;
}

void InfoBox::setInfoLineSlot(int line, QString text)
{
    if(line < 0 || line > 2)
        return;
    setCellContent(line, 0, text);
}
