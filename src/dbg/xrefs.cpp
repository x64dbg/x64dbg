#include "xrefs.h"
#include "addrinfo.h"

struct XREFSINFO : AddrInfo
{
    XREFTYPE type = XREF_NONE;
    std::unordered_map<duint, XREF_RECORD> references;
};

struct XrefSerializer : AddrInfoSerializer<XREFSINFO>
{
    bool Save(const XREFSINFO & value) override
    {
        AddrInfoSerializer::Save(value);
        auto references = json_array();
        for(const auto & itr : value.references)
        {
            auto reference = json_object();
            json_object_set_new(reference, "addr", json_hex(itr.second.addr));
            json_object_set_new(reference, "type", json_hex(itr.second.type));
            json_array_append_new(references, reference);
        }
        set("references", references);
        return true;
    }

    bool Load(XREFSINFO & value) override
    {
        if(!AddrInfoSerializer::Load(value))
            return false;
        auto references = get("references");
        if(!references)
            return false;
        value.type = XREF_DATA;
        size_t i;
        JSON reference;
        json_array_foreach(references, i, reference)
        {
            XREF_RECORD record;
            record.addr = duint(json_hex_value(json_object_get(reference, "addr")));
            record.type = XREFTYPE(json_hex_value(json_object_get(reference, "type")));
            value.type = max(record.type, value.type);
            value.references.insert({ record.addr, record });
        }
        return true;
    }
};

struct Xrefs : AddrInfoHashMap<LockCrossReferences, XREFSINFO, XrefSerializer>
{
    const char* jsonKey() const override
    {
        return "xrefs";
    }
};

static Xrefs xrefs;

bool XrefAdd(duint Address, duint From)
{
    XREFSINFO info;
    if(!MemIsValidReadPtr(From) || !xrefs.PrepareValue(info, Address, false))
        return false;

    // Fail if boundary exceeds module size
    auto moduleBase = ModBaseFromAddr(Address);
    if(moduleBase != ModBaseFromAddr(From))
        return false;

    BASIC_INSTRUCTION_INFO instInfo;
    DbgDisasmFastAt(From, &instInfo);

    XREF_RECORD xrefRecord;
    xrefRecord.addr = From - moduleBase;
    if(instInfo.call)
        xrefRecord.type = XREF_CALL;
    else if(instInfo.branch)
        xrefRecord.type = XREF_JMP;
    else
        xrefRecord.type = XREF_DATA;

    EXCLUSIVE_ACQUIRE(LockCrossReferences);
    auto & mapData = xrefs.GetDataUnsafe();
    auto key = Xrefs::VaKey(Address);
    auto found = mapData.find(key);
    if(found == mapData.end())
    {
        info.type = xrefRecord.type;
        info.references.insert({ xrefRecord.addr, xrefRecord });
        mapData.insert({ key, info });
    }
    else
    {
        found->second.references.insert({ xrefRecord.addr, xrefRecord });
        found->second.type = max(found->second.type, xrefRecord.type);
    }
    return true;
}

bool XrefGet(duint Address, XREF_INFO* List)
{
    SHARED_ACQUIRE(LockCrossReferences);
    auto & mapData = xrefs.GetDataUnsafe();
    auto found = mapData.find(Xrefs::VaKey(Address));
    if(found == mapData.end())
        return false;
    if(List->refcount != found->second.references.size())
        return false;
    auto moduleBase = ModBaseFromAddr(Address);
    auto ptr = List->references;
    for(const auto & itr : found->second.references)
    {
        *ptr = itr.second;
        ptr->addr += moduleBase;
        ++ptr;
    }
    return true;
}

duint XrefGetCount(duint Address)
{
    SHARED_ACQUIRE(LockCrossReferences);
    auto & mapData = xrefs.GetDataUnsafe();
    auto found = mapData.find(Xrefs::VaKey(Address));
    return found == mapData.end() ? 0 : found->second.references.size();
}

XREFTYPE XrefGetType(duint Address)
{
    SHARED_ACQUIRE(LockCrossReferences);
    auto & mapData = xrefs.GetDataUnsafe();
    auto found = mapData.find(Xrefs::VaKey(Address));
    return found == mapData.end() ? XREF_NONE : found->second.type;
}

bool XrefDeleteAll(duint Address)
{
    return xrefs.Delete(Xrefs::VaKey(Address));
}

void XrefDelRange(duint Start, duint End)
{
    xrefs.DeleteRange(Start, End, false);
}

void XrefCacheSave(JSON Root)
{
    xrefs.CacheSave(Root);
}

void XrefCacheLoad(JSON Root)
{
    xrefs.CacheLoad(Root);
}

void XrefClear()
{
    xrefs.Clear();
}
