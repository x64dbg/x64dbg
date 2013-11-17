#ifndef CPUDISASSEMBLY_H
#define CPUDISASSEMBLY_H

#include <QtGui>
#include <QtDebug>
#include "NewTypes.h"
#include "Disassembly.h"


class CPUDisassembly : public Disassembly
{
    Q_OBJECT
public:
    explicit CPUDisassembly(QWidget *parent = 0);
    
signals:
    
public slots:
    
};

#endif // CPUDISASSEMBLY_H
