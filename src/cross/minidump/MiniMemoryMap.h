#pragma once

#include <BasicView/StdTable.h>
#include "FileParser.h"
#include "MagicMenu.h"
#include "Navigation.h"

class MiniMemoryMap : public StdTable, public MagicMenu<MiniMemoryMap>
{
    Q_OBJECT

public:
    explicit MiniMemoryMap(Navigation* navigation, QWidget* parent = nullptr);
    void loadFileParser(FileParser* parser);
    void gotoAddress(duint address);
    duint selectedAddress();

private:
    FileParser* mParser = nullptr;
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
