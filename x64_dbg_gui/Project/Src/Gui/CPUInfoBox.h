#ifndef INFOBOX_H
#define INFOBOX_H

#include "StdTable.h"

class CPUInfoBox : public StdTable
{
    Q_OBJECT
public:
    explicit CPUInfoBox(StdTable* parent = 0);
    int getHeight();
    void addFollowMenuItem(QMenu* menu, QString name, int_t value);
    void setupFollowMenu(QMenu* menu, int_t wVA);

public slots:
    void disasmSelectionChanged(int_t parVA);
    void dbgStateChanged(DBGSTATE state);
    void contextMenuSlot(QPoint pos);
    void followActionSlot();

private:
    int_t curAddr;
    QString getSymbolicName(int_t addr);
    void setInfoLine(int line, QString text);
    QString getInfoLine(int line);
    void clear();
};

#endif // INFOBOX_H
