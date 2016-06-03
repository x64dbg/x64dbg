#include "label.h"
#include "threading.h"
#include "module.h"
#include "memory.h"
#include "console.h"

struct CommentSerializer : JSONWrapper<LABELSINFO>
{
    bool Save(const LABELSINFO & value) override
    {
        setString("module", value.mod);
        setHex("address", value.addr);
        setString("text", value.text);
        setBool("manual", value.manual);
        return true;
    }

    bool Load(LABELSINFO & value) override
    {
        value.manual = true;
        getBool("manual", value.manual); //legacy support
        return getString("module", value.mod) &&
               getHex("address", value.addr) &&
               getString("text", value.text);
    }
};

struct Labels : SerializableModuleHashMap<LockComments, LABELSINFO, CommentSerializer>
{
    void AdjustValue(LABELSINFO & value) const override
    {
        value.addr += ModBaseFromName(value.mod);
    }

protected:
    const char* jsonKey() const override
    {
        return "labels";
    }

    duint makeKey(const LABELSINFO & value) const override
    {
        return ModHashFromName(value.mod) + value.addr;
    }
};

static Labels labels;

bool LabelSet(duint Address, const char* Text, bool Manual)
{
    // A valid memory address must be supplied
    if(!MemIsValidReadPtr(Address))
        return false;

    // Make sure the string is supplied, within bounds, and not a special delimiter
    if(!Text || Text[0] == '\1' || strlen(Text) >= MAX_LABEL_SIZE - 1)
        return false;

    // Labels cannot be "address" of actual variables
    if(strstr(Text, "&"))
        return false;

    // Delete the label if no text was supplied
    if(Text[0] == '\0')
    {
        LabelDelete(Address);
        return true;
    }

    // Fill out the structure data
    LABELSINFO labelInfo;
    labelInfo.manual = Manual;
    labelInfo.addr = Address - ModBaseFromAddr(Address);
    strcpy_s(labelInfo.text, Text);
    if(!ModNameFromAddr(Address, labelInfo.mod, true))
        *labelInfo.mod = '\0';

    return labels.Add(labelInfo);
}

bool LabelFromString(const char* Text, duint* Address)
{
    return labels.GetWhere([&](const LABELSINFO & value)
    {
        if(strcmp(value.text, Text))
            return false;
        if(Address)
            *Address = value.addr + ModBaseFromName(value.mod);
        return true;
    });
}

bool LabelGet(duint Address, char* Text)
{
    LABELSINFO label;
    if(!labels.Get(Labels::VaKey(Address), label))
        return false;
    if(Text)
        strcpy_s(Text, MAX_LABEL_SIZE, label.text);
    return true;
}

bool LabelDelete(duint Address)
{
    return labels.Delete(Labels::VaKey(Address));
}

void LabelDelRange(duint Start, duint End, bool Manual)
{
    labels.DeleteRange(Start, End, [Manual](duint start, duint end, const LABELSINFO & value)
    {
        if(Manual ? !value.manual : value.manual)  //ignore non-matching entries
            return false;
        return value.addr >= start && value.addr < end;
    });
}

void LabelCacheSave(JSON Root)
{
    labels.CacheSave(Root);
}

void LabelCacheLoad(JSON Root)
{
    labels.CacheLoad(Root);
    labels.CacheLoad(Root, false, "auto"); //legacy support
}

bool LabelEnum(LABELSINFO* List, size_t* Size)
{
    return labels.Enum(List, Size);
}

void LabelClear()
{
    labels.Clear();
}

void LabelGetList(std::vector<LABELSINFO> & list)
{
    labels.GetList(list);
}

bool LabelGetInfo(duint Address, LABELSINFO* info)
{
    return labels.GetInfo(Address, info);
}
