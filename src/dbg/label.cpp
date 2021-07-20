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
static std::unordered_map<duint, std::string> tempLabels;

bool LabelSet(duint Address, const char* Text, bool Manual, bool Temp)
{
    // Make sure the string is supplied, within bounds, and not a special delimiter
    if(!Text || Text[0] == '\1' || strlen(Text) >= MAX_LABEL_SIZE - 1)
        return false;
    // Delete the label if no text was supplied
    if(Text[0] == '\0')
    {
        LabelDelete(Address);
        return true;
    }
    if(Temp)
    {
        tempLabels[Address] = Text;
        return true;
    }
    // Fill in the structure + add to database
    LABELSINFO label;
    if(!labels.PrepareValue(label, Address, Manual))
        return false;
    label.text = Text;
    return labels.Add(label);
}

bool LabelFromString(const char* Text, duint* Address)
{
    auto found = labels.GetWhere([&](const LABELSINFO & value)
    {
        if(strcmp(value.text.c_str(), Text))
            return false;
        if(Address)
            *Address = value.addr + ModBaseFromName(value.mod().c_str());
        return true;
    });
    if(!found)
    {
        for(auto & label : tempLabels)
            if(strcmp(label.second.c_str(), Text) == 0)
            {
                if(Address)
                    *Address = label.first;
                return true;
            }
    }
    return found;
}

bool LabelGet(duint Address, char* Text)
{
    LABELSINFO label;
    if(!labels.Get(Labels::VaKey(Address), label))
    {
        auto found = tempLabels.find(Address);
        if(found == tempLabels.end())
            return false;
        if(Text)
            strncpy_s(Text, MAX_LABEL_SIZE, found->second.c_str(), _TRUNCATE);
        return true;
    }
    if(Text)
        strncpy_s(Text, MAX_LABEL_SIZE, label.text.c_str(), _TRUNCATE);
    return true;
}

bool LabelDelete(duint Address)
{
    return labels.Delete(Labels::VaKey(Address)) || tempLabels.erase(Address) > 0;
}

void LabelDelRange(duint Start, duint End, bool Manual)
{
    labels.DeleteRange(Start, End, Manual);
    if(Start == 0 && End == ~0)
    {
        tempLabels.clear();
    }
    else
    {
        for(auto it = tempLabels.begin(); it != tempLabels.end();)
        {
            if(it->first >= Start && it->first < End)
                it = tempLabels.erase(it);
            else
                ++it;
        }
    }
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

void LabelClear()
{
    labels.Clear();
    tempLabels.clear();
}

void LabelGetList(std::vector<LABELSINFO> & list)
{
    labels.GetList(list);
    list.reserve(list.size() + tempLabels.size());
    for(auto & label : tempLabels)
    {
        LABELSINFO info;
        info.modhash = ModHashFromAddr(label.first);
        info.addr = label.first;
        info.manual = false;
        info.text = label.second;
        list.push_back(info);
    }
}

bool LabelGetInfo(duint Address, LABELSINFO* info)
{
    if(info == nullptr)
        return false;

    return labels.Get(Labels::VaKey(Address), *info);
}
