#include "TraceRegisters.h"

TraceRegisters::TraceRegisters(QWidget* parent) : RegistersView(parent)
{

}

void TraceRegisters::setRegisters(REGDUMP* registers)
{
    this->RegistersView::setRegisters(registers);
}

void TraceRegisters::setActive(bool isActive)
{
    this->isActive = isActive;
    this->RegistersView::setRegisters(&this->wRegDumpStruct);
}
