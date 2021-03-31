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

public slots:
    virtual void displayCustomContextMenuSlot(QPoint pos);
    void onCopySIMDRegister();

protected:
    virtual void mouseDoubleClickEvent(QMouseEvent* event);

private:
    QAction* wCM_CopySIMDRegister;
};

#endif // TRACEREGISTERS_H
