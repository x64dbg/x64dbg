#include "label.h"

struct LabelSerializer : AddrInfoSerializer<LABELSINFO>
{
    bool Save(const LABELSINFO & value) override
    {
        AddrInfoSerializer::Save(value);
        setString("text", value.text);
        return true;
    }

    bool Load(LABELSINFO & value) override
    {
        return AddrInfoSerializer::Load(value) &&
               getString("text", value.text);
    }
};

struct Labels : AddrInfoHashMap<LockLabels, LABELSINFO, LabelSerializer>
{
    const char* jsonKey() const override
    {
        return "labels";
    }
};

static Labels labels;

bool LabelSet(duint Address, const char* Text, bool Manual)
{
    // Make sure the string is supplied, within bounds, and not a special delimiter
    if(!Text || Text[0] == '\1' || strlen(Text) >= MAX_LABEL_SIZE - 1 || strstr(Text, "&"))
        return false;
    // Delete the label if no text was supplied
    if(Text[0] == '\0')
    {
        LabelDelete(Address);
        return true;
    }
    // Fill in the structure + add to database
    LABELSINFO label;
    if(!labels.PrepareValue(label, Address, Manual))
        return false;
    strcpy_s(label.text, Text);
    return labels.Add(label);
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
    labels.DeleteRange(Start, End, Manual);
}

void LabelCacheSave(JSON Root)
{
    labels.CacheSave(Root);
}

void LabelCacheLoad(JSON Root)
{
    labels.CacheLoad(Root);
    labels.CacheLoad(Root, "auto"); //legacy support
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
