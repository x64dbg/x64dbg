#pragma once

#include <BasicView/StdTable.h>
#include "MiniDump.h"
#include "MagicMenu.h"
#include "Navigation.h"

class MiniMemoryMap : public StdTable, public MagicMenu<MiniMemoryMap>
{
    Q_OBJECT

public:
    explicit MiniMemoryMap(Navigation* navigation, QWidget* parent = nullptr);
    void loadMinidump(MiniDump::AbstractParser* parser);
    void gotoAddress(duint address);
    duint selectedAddress();

private:
    MiniDump::AbstractParser* mParser = nullptr;
    Navigation* mNavigation = nullptr;

    enum ColumnIndex
    {
        ColAllocationBase,
        ColBaseAddress,
        ColRegionSize,
        ColType,
        ColProtect,
        ColAllocationProtect,
        ColState,
        ColInfo,
    };

    void setupMenu();
};
