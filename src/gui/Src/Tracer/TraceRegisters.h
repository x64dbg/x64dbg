#ifndef TRACEREGISTERS_H
#define TRACEREGISTERS_H
#include "RegistersView.h"

class TraceRegisters : public RegistersView
{
    Q_OBJECT
public:
    TraceRegisters(QWidget* parent = 0);

    void setRegisters(REGDUMP* registers);
    void setActive(bool isActive);

    void mousePressEvent(QMouseEvent* event);

public slots:
    virtual void displayCustomContextMenuSlot(QPoint pos);
    void onCopySSERegister();

private:
    QAction* wCM_CopySSERegister;
};

#endif // TRACEREGISTERS_H
