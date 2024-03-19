#include "Navigation.h"

Navigation::Navigation(QObject* parent)
    : QObject(parent)
{
}

void Navigation::gotoDump(duint address)
{
    gotoFocus(Window::Dump, address);
}

void Navigation::gotoDisassembly(duint address)
{
    gotoFocus(Window::Disassembly, address);
}

void Navigation::gotoMemoryMap(duint address)
{
    gotoFocus(Window::MemoryMap, address);
}

void Navigation::gotoFocus(Window window, duint address)
{
    emit gotoAddress(window, address);
    emit focusWindow(window);
}
