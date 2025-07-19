#pragma once

#include <QObject>
#include "Types.h"

class Navigation : public QObject
{
    Q_OBJECT
public:
    explicit Navigation(QObject* parent = nullptr);

    enum Window
    {
        Dump,
        Disassembly,
        MemoryMap,
    };

    void gotoDump(duint address);
    void gotoDisassembly(duint address);
    void gotoMemoryMap(duint address);

signals:
    void gotoAddress(Navigation::Window window, duint address);
    void focusWindow(Navigation::Window window);

private:
    void gotoFocus(Window window, duint address);
};
