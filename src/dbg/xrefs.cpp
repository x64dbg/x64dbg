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
        value.references.reserve(json_array_size(references));
        json_array_foreach(references, i, reference)
        {
            XREF_RECORD record;
            record.addr = duint(json_hex_value(json_object_get(reference, "addr")));
            record.type = XREFTYPE(json_hex_value(json_object_get(reference, "type")));
            value.type = std::max(record.type, value.type);
            value.references.emplace(record.addr, record);
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
    XREF_EDGE edge = { Address, From };
    return XrefAddMulti(&edge, 1) == 1;
}

duint XrefAddMulti(const XREF_EDGE* Edges, duint Count)
{
    // These types are used in a cache to improve performance
    struct FromInfo
    {
        bool valid = false;
        duint moduleBase = 0;
        duint moduleSize = 0;
        XREF_RECORD xrefRecord{};

        explicit FromInfo(duint from)
        {
            if(!MemIsValidReadPtr(from))
                return;

            {
                SHARED_ACQUIRE(LockModules);

                auto module = ModInfoFromAddr(from);
                if(!module)
                    return;

                moduleBase = module->base;
                moduleSize = module->size;
            }

            BASIC_INSTRUCTION_INFO instInfo;
            DbgDisasmFastAt(from, &instInfo);

            xrefRecord.addr = from - moduleBase;
            if(instInfo.call)
                xrefRecord.type = XREF_CALL;
            else if(instInfo.branch)
                xrefRecord.type = XREF_JMP;
            else
                xrefRecord.type = XREF_DATA;

            valid = true;
        }
    };

    struct AddressInfo
    {
        bool valid = false;
        XREFSINFO* info = nullptr;

        explicit AddressInfo(duint address)
        {
            XREFSINFO preparedInfo;
            if(!xrefs.PrepareValue(preparedInfo, address, false))
                return;

            auto key = Xrefs::VaKey(address);
            auto & mapData = xrefs.GetDataUnsafe();
            auto insertResult = mapData.emplace(key, preparedInfo);

            info = &insertResult.first->second;

            valid = true;
        }
    };

    EXCLUSIVE_ACQUIRE(LockCrossReferences);

    std::unordered_map<duint, FromInfo> fromCache;
    std::unordered_map<duint, AddressInfo> addressCache;
    duint succeeded = 0;

    for(duint i = 0; i < Count; i++)
    {
        duint address = Edges[i].address;
        duint from = Edges[i].from;

        auto fromCacheIt = fromCache.find(from);
        if(fromCacheIt == fromCache.end())
            fromCacheIt = fromCache.emplace(from, FromInfo(from)).first;

        const auto & fromInfo = fromCacheIt->second;
        if(!fromInfo.valid)
            continue;
        if(address < fromInfo.moduleBase || address >= fromInfo.moduleBase + fromInfo.moduleSize)
            continue;

        auto addressCacheIt = addressCache.find(address);
        if(addressCacheIt == addressCache.end())
            addressCacheIt = addressCache.emplace(address, AddressInfo(address)).first;

        const auto & addressInfo = addressCacheIt->second;
        if(!addressInfo.valid)
            continue;

        auto & info = *addressInfo.info;
        auto & xrefRecord = fromInfo.xrefRecord;
        info.references.emplace(xrefRecord.addr, xrefRecord);
        info.type = std::max(info.type, xrefRecord.type);

        succeeded++;
    }

    return succeeded;
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
