#pragma once

#include <BasicView/HexDump.h>
#include "MiniDump.h"
#include "MagicMenu.h"
#include "Navigation.h"

class MiniHexDump : public HexDump, public MagicMenu<MiniHexDump>
{
    Q_OBJECT

public:
    explicit MiniHexDump(Navigation* navigation, Architecture* architecture, QWidget* parent = nullptr);
    void loadMinidump(MiniDump::AbstractParser* parser);

private slots:
    void hexAsciiSlot();

private:
    MiniDump::AbstractParser* mParser = nullptr;
    int mAsciiSeparator = 0;
    Navigation* mNavigation = nullptr;

    void setupMenu();
};
