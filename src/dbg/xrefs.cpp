#include "xrefs.h"
#include "module.h"
#include "memory.h"
#include "threading.h"

struct XrefInfo_t
{
    char mod[MAX_MODULE_SIZE];
    duint address;
    std::unordered_set<duint> references;
    size_t jmp_count;
    size_t call_count;
};

std::unordered_map<duint, XrefInfo_t> references;

bool XrefAdd(duint Address, duint From, XREFTYPE type)
{
    ASSERT_DEBUGGING("Export call");


    // Make sure memory is readable
    if(!MemIsValidReadPtr(Address))
        return false;

    // Fail if boundary exceeds module size
    const duint moduleBase = ModBaseFromAddr(Address);

    if(moduleBase != ModBaseFromAddr(From))
        return false;

    duint addrHash = ModHashFromAddr(Address);



    // Insert to global table
    EXCLUSIVE_ACQUIRE(LockCrossReferences);
    if(references.find(addrHash) == references.end())
    {
        XrefInfo_t xref;
        ModNameFromAddr(Address, xref.mod, true);
        xref.address = Address - moduleBase;
        xref.jmp_count = 0;
        xref.call_count = 0;
        xref.references.insert(From);

        references[addrHash] = xref;
    }
    else
    {
        references[addrHash].references.insert(From);
    }
    if(type == XREF_JMP)
        references[addrHash].jmp_count++;
    else if(type == XREF_CALL)
        references[addrHash].call_count++;

    return true;
}

bool XrefGet(duint Address, XREF_INFO* List)
{
    ASSERT_DEBUGGING("Export call");

    const duint moduleBase = ModBaseFromAddr(Address);

    duint addrHash = ModHashFromAddr(Address);

    SHARED_ACQUIRE(LockCrossReferences);

    auto found = references.find(addrHash);


    if(found == references.end())
        return false;

    if(List->refcount != found->second.references.size())
    {

        return false;
    }


    strcpy_s(List->mod, found->second.mod);

    duint* ptr = List->references;

    for(auto iter = found->second.references.begin(); iter != found->second.references.end(); iter++)
    {
        *ptr++ = *iter;
    }


    return true;
}

size_t XrefGetCount(duint Address)
{
    ASSERT_DEBUGGING("Export call");

    const duint addressHash = ModHashFromAddr(Address);

    SHARED_ACQUIRE(LockCrossReferences);
    auto found = references.find(addressHash);
    return found == references.end() ? 0 : found->second.references.size();
}

XREFTYPE XrefGetType(duint Address)
{
    ASSERT_DEBUGGING("Export call");

    const duint addressHash = ModHashFromAddr(Address);

    SHARED_ACQUIRE(LockCrossReferences);
    auto found = references.find(addressHash);
    if(found == references.end())
        return XREF_NONE;
    else if(found->second.jmp_count > 0)
        return XREF_JMP;
    else if(found->second.call_count > 0)
        return XREF_CALL;
    return XREF_NONE;
}

bool XrefDeleteAll(duint Address)
{
    ASSERT_DEBUGGING("Export call");

    const duint addressHash = ModHashFromAddr(Address);

    EXCLUSIVE_ACQUIRE(LockCrossReferences);
    //auto found = references.find(Address);

    return (references.erase(addressHash) > 0);
}

void XrefDelRange(duint Start, duint End, bool DeleteManual)
{
    ASSERT_DEBUGGING("Export call");

    // Should all functions be deleted?
    // 0x00000000 - 0xFFFFFFFF
    if(Start == 0 && End == ~0)
    {
        XrefClear();
    }
    else
    {
        // The start and end address must be in the same module
        duint moduleBase = ModBaseFromAddr(Start);

        if(moduleBase != ModBaseFromAddr(End))
            return;

        // Convert these to a relative offset
        Start -= moduleBase;
        End -= moduleBase;

        EXCLUSIVE_ACQUIRE(LockCrossReferences);
        for(auto itr = references.begin(); itr != references.end();)
        {
            const auto & currentAddr = itr->second.address;

            // [Start, End]
            if(currentAddr >= Start && currentAddr <= End)
                itr = references.erase(itr);
            else
                ++itr;
        }
    }
}

void XrefCacheSave(JSON Root)
{
    EXCLUSIVE_ACQUIRE(LockCrossReferences);

    // Allocate JSON object array
    const JSON jsonXrefs = json_array();


    for(auto & i : references)
    {
        JSON currentRecord = json_object();

        json_object_set_new(currentRecord, "module", json_string(i.second.mod));
        json_object_set_new(currentRecord, "address", json_hex(i.second.address));
        json_object_set_new(currentRecord, "jmp_count", json_hex(i.second.jmp_count));
        json_object_set_new(currentRecord, "call_count", json_hex(i.second.call_count));

        JSON references = json_array();
        for(auto & addr : i.second.references)
        {
            json_array_append_new(references, json_hex(addr));
        }

        json_object_set_new(currentRecord, "references", references);

        json_array_append_new(jsonXrefs, currentRecord);

    }

    if(json_array_size(jsonXrefs))
        json_object_set(Root, "xrefs", jsonXrefs);

    // Decrease reference count to avoid leaking memory

    json_decref(jsonXrefs);
}

void XrefCacheLoad(JSON Root)
{
    EXCLUSIVE_ACQUIRE(LockCrossReferences);

    // Delete existing entries
    references.clear();

    const JSON jsonXrefs = json_object_get(Root, "xrefs");

    size_t i, j;
    JSON value, addr;
    json_array_foreach(jsonXrefs, i, value)
    {
        XrefInfo_t xrefinfo;
        memset(&xrefinfo, 0, sizeof(XrefInfo_t));

        // Copy module name
        const char* mod = json_string_value(json_object_get(value, "module"));

        if(mod && *mod && strlen(mod) < MAX_MODULE_SIZE)
            strcpy_s(xrefinfo.mod, mod);

        // Function address
        xrefinfo.address = (duint)json_hex_value(json_object_get(value, "address"));
        xrefinfo.jmp_count = (duint)json_hex_value(json_object_get(value, "jmp_count"));
        xrefinfo.call_count = (duint)json_hex_value(json_object_get(value, "call_count"));

        json_array_foreach(jsonXrefs, j, addr)
        {
            xrefinfo.references.insert((duint)json_hex_value(addr));
        }

        const duint key = ModHashFromName(xrefinfo.mod) + xrefinfo.address;
        references[key] = xrefinfo;
    }



}

bool XrefEnum(XREF_INFO* List, size_t* Size)
{
    ASSERT_DEBUGGING("Export call");

    // If a list isn't passed and the size not requested, fail
    ASSERT_FALSE(!List && !Size);
    SHARED_ACQUIRE(LockCrossReferences);

    //// Did the caller request the buffer size needed?
    //if (Size)
    //{
    //  *Size = functions.size() * sizeof(FUNCTIONSINFO);

    //  if (!List)
    //      return true;
    //}

    //// Fill out the buffer
    //for (auto & itr : functions)
    //{
    //  // Adjust for relative to virtual addresses
    //  duint moduleBase = ModBaseFromName(itr.second.mod);

    //  *List = itr.second;
    //  List->start += moduleBase;
    //  List->end += moduleBase;

    //  List++;
    //}

    return true;
}

void XrefClear()
{
    EXCLUSIVE_ACQUIRE(LockCrossReferences);
    references.clear();
}

void XrefGetList(std::vector<XREF_INFO> & list)
{
    SHARED_ACQUIRE(LockCrossReferences);
    list.clear();
    list.reserve(references.size());
    //  for (const auto & itr : references)
    //  list.push_back(itr.second);
}

bool XrefGetInfo(duint Address, XREF_INFO* info)
{
    auto moduleBase = ModBaseFromAddr(Address);

    // Lookup by module hash, then function range
    SHARED_ACQUIRE(LockCrossReferences);

    auto found = references.find(Address);

    // Was this range found?
    if(found == references.end())
        return false;

    return true;
}
