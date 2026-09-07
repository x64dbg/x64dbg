#include "addresscolor.h"
#include "database_cb_batcher.h"

struct AddressColorSerializer : RangeInfoSerializer<ADDRESSCOLORINFO>
{
    bool Save(const ADDRESSCOLORINFO & value) override
    {
        RangeInfoSerializer::Save(value);
        setHex("color", value.color);
        return true;
    }

    bool Load(ADDRESSCOLORINFO & value) override
    {
        return loadRangeInfo(value, true) &&
               getHex("color", value.color);
    }
};

struct AddressColors : SplitRangeInfoMap<LockAddressColors, ADDRESSCOLORINFO, AddressColorSerializer>
{
    const char* jsonKey() const override
    {
        return "addresscolors";
    }

protected:
    bool populateDbOperation(DbOperation & op, const ADDRESSCOLORINFO & value) const override
    {
        op.itemType = DbItemTypeAddressColor;
        op.manual = value.manual;
        op.modhash = value.modhash;
        op.address = value.start;
        op.addressColor.end = value.end;
        op.addressColor.color = value.color;
        return true;
    }
};

static AddressColors addressColors;

static bool setRange(duint Start, duint End, duint color, bool Manual)
{
    ADDRESSCOLORINFO info;
    if(!MemIsValidReadPtr(End) || !addressColors.PrepareValue(info, Start, End, Manual))
        return false;
    info.color = color;
    return addressColors.ReplaceRange(info);
}

bool AddressColorSet(duint Address, duint color, bool Manual)
{
    return setRange(Address, Address, color, Manual);
}

bool AddressColorSetRange(duint Start, duint End, duint color, bool Manual)
{
    if(Start == End)
        return setRange(Start, End, color, Manual);
    DbCallbackBatcher batcher;
    return setRange(Start, End, color, Manual);
}

bool AddressColorGet(duint Address, duint* color)
{
    ADDRESSCOLORINFO info;
    if(!addressColors.Get(AddressColors::VaKey(Address, Address), info))
        return false;
    if(color)
        *color = info.color;
    return true;
}

bool AddressColorDelete(duint Address)
{
    return addressColors.DeleteRangeWhere(Address, Address, [](const ADDRESSCOLORINFO &)
    {
        return true;
    });
}

void AddressColorDelRange(duint Start, duint End, bool Manual)
{
    DbCallbackBatcher batcher;
    addressColors.DeleteRangeWhere(Start, End, [Manual](const ADDRESSCOLORINFO & value)
    {
        return value.manual == Manual;
    });
}

void AddressColorCacheSave(JSON Root)
{
    addressColors.CacheSave(Root);
}

void AddressColorCacheLoad(JSON Root)
{
    DbCallbackBatcher batcher(true);
    addressColors.CacheLoad(Root);
}

bool AddressColorEnum(ADDRESSCOLORINFO* List, size_t* Size)
{
    return addressColors.Enum(List, Size);
}

void AddressColorClear(bool Terminating)
{
    DbCallbackBatcher batcher;
    addressColors.Clear(Terminating);
}

void AddressColorGetList(std::vector<ADDRESSCOLORINFO> & list)
{
    addressColors.GetList(list);
}

bool AddressColorGetInfo(duint Address, ADDRESSCOLORINFO* info)
{
    return addressColors.GetInfo(AddressColors::VaKey(Address, Address), info);
}
