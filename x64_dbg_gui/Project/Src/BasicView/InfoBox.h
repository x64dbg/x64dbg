#ifndef INFOBOX_H
#define INFOBOX_H

#include <QtGui>
#include "StdTable.h"
#include "Bridge.h"
#include "CPUDisassembly.h"

class InfoBox : public StdTable
{
    Q_OBJECT
public:
    explicit InfoBox(StdTable *parent = 0);
    int getHeight();

public slots:
    void disasmSelectionChanged(int_t parVA);

private:
    void setInfoLine(int line, QString text);
};

#endif // INFOBOX_H
