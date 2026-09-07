#include "cmd-addresscolor.h"
#include "addresscolor.h"
#include "value.h"
#include "console.h"

static unsigned int parseColor(const char* colorStr)
{
    duint count = 0;
    if(!BridgeSettingGetUint("Colors", "AddressColorCount", &count) || count == 0)
        count = 6;

    duint color = DbgValFromString(colorStr);
    if(color >= 1 && color <= count)
        return (unsigned int)color;

    dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid color preset '%s' (expected: 1-%u)\n"), colorStr, (unsigned int)count);
    return 0;
}

bool cbDebugAddressColorSet(int argc, char* argv[])
{
    if(argc < 3)
        return false;
    duint address = DbgValFromString(argv[1]);
    unsigned int color = parseColor(argv[2]);
    if(color == 0 || !AddressColorSet(address, color, true))
        return false;
    GuiUpdateDisassemblyView();
    return true;
}

bool cbDebugAddressColorSetRange(int argc, char* argv[])
{
    if(argc < 4)
        return false;
    duint start = DbgValFromString(argv[1]);
    duint end = DbgValFromString(argv[2]);
    unsigned int color = parseColor(argv[3]);
    if(color == 0)
        return false;

    if(!AddressColorSetRange(start, end, color, true))
        return false;
    GuiUpdateDisassemblyView();
    return true;
}

bool cbDebugAddressColorDelete(int argc, char* argv[])
{
    if(argc < 2)
        return false;

    duint start = DbgValFromString(argv[1]);
    if(argc >= 3)
    {
        duint end = DbgValFromString(argv[2]);
        AddressColorDelRange(start, end, true);
    }
    else if(!AddressColorDelete(start))
    {
        return false;
    }
    GuiUpdateDisassemblyView();
    return true;
}

bool cbDebugAddressColorClear(int argc, char* argv[])
{
    AddressColorClear(false);
    GuiUpdateDisassemblyView();
    return true;
}
