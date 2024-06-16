#pragma once

#include "MiniDump.h"
#include <BasicView/Disassembly.h>
#include "MagicMenu.h"
#include "Navigation.h"

class MiniDisassembly : public Disassembly, public MagicMenu<MiniDisassembly>
{
    Q_OBJECT

public:
    MiniDisassembly(Navigation* navigation, Architecture* architecture, QWidget* parent = nullptr);
    void loadMinidump(const MiniDump::AbstractParser* parser);

private:
    const MiniDump::AbstractParser* mParser = nullptr;
    Navigation* mNavigation = nullptr;

    void setupMenu();
};
