#pragma once
#include "Bridge.h"
#include <QString>

class QWidget;
class QMenu;

namespace KSwordEngine
{
    bool selected();
    QString mechanismName(uint32_t mechanism);
    bool breakpointInfo(duint address, BPXTYPE type, KSWORD_DEBUGGER_BREAKPOINT_INFO& info, DWORD* error = nullptr);
    QString breakpointLabel(duint address, BPXTYPE type);
    void showBreakpoint(QWidget* parent, duint address, BPXTYPE type);
    void showAddress(QWidget* parent, duint address);
    void addMenu(QMenu* menu, QWidget* parent);
}
