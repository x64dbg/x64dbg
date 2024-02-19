#pragma once

#include "RegistersView.h"

class TraceRegisters : public RegistersView
{
    Q_OBJECT
public:
    TraceRegisters(QWidget* parent = nullptr);

    void setRegisters(REGDUMP* registers);
    void setActive(bool isActive);

public slots:
    virtual void displayCustomContextMenuSlot(QPoint pos);
    void onCopySIMDRegister();
    void onSetRegister();

protected:
    virtual void mouseDoubleClickEvent(QMouseEvent* event);

private:
    QAction* wCM_CopySIMDRegister;
    QAction* wCM_SetRegister;
};
