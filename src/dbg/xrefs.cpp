#include "xrefs.h"
#include "module.h"
#include "memory.h"
#include "threading.h"

struct XrefInfo_t
{
    char mod[MAX_MODULE_SIZE];
    duint address;
    std::unordered_map<duint, XREF_RECORD> references;
    size_t jmp_count;
    size_t call_count;
};

std::unordered_map<duint, XrefInfo_t> references;

bool XrefAdd(duint Address, duint From)
{

    // Make sure memory is readable
    if(!MemIsValidReadPtr(Address))
        return false;

    // Fail if boundary exceeds module size
    const duint moduleBase = ModBaseFromAddr(Address);

    if(moduleBase != ModBaseFromAddr(From))
        return false;

    duint addrHash = ModHashFromAddr(Address);

    BASIC_INSTRUCTION_INFO instInfo;
    DbgDisasmFastAt(From, &instInfo);

    XREF_RECORD xrefRecord;
    xrefRecord.addr = From - moduleBase;

    strtok(instInfo.instruction, " ");
    strtok(NULL, " ");
    strcpy_s(xrefRecord.inst, instInfo.instruction);


    // Insert to global table
    EXCLUSIVE_ACQUIRE(LockCrossReferences);
    if(references.find(addrHash) == references.end())
    {
        XrefInfo_t xref;
        ModNameFromAddr(Address, xref.mod, true);
        xref.address = Address - moduleBase;
        xref.jmp_count = 0;
        xref.call_count = 0;
        xref.references[From] = xrefRecord;

        references[addrHash] = xref;
    }
    else
    {
        references[addrHash].references[From] = xrefRecord;
    }
    if(instInfo.call)
        references[addrHash].call_count++;
    else if(instInfo.branch)
        references[addrHash].jmp_count++;

    return true;
}

bool XrefGet(duint Address, XREF_INFO* List)
{
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

    XREF_RECORD* ptr = List->references;

    for(auto iter = found->second.references.begin(); iter != found->second.references.end(); iter++)
    {
        ptr->addr = iter->first + moduleBase;
        strcpy_s(ptr->inst, iter->second.inst);
        ptr++;
    }


    return true;
}

size_t XrefGetCount(duint Address)
{
    const duint addressHash = ModHashFromAddr(Address);

    SHARED_ACQUIRE(LockCrossReferences);
    auto found = references.find(addressHash);
    return found == references.end() ? 0 : found->second.references.size();
}

XREFTYPE XrefGetType(duint Address)
{

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

    const duint addressHash = ModHashFromAddr(Address);

    EXCLUSIVE_ACQUIRE(LockCrossReferences);
    //auto found = references.find(Address);

    return (references.erase(addressHash) > 0);
}

void XrefDelRange(duint Start, duint End, bool DeleteManual)
{

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
        JSON currentXref = json_object();

        json_object_set_new(currentXref, "module", json_string(i.second.mod));
        json_object_set_new(currentXref, "address", json_hex(i.second.address));
        json_object_set_new(currentXref, "jmp_count", json_hex(i.second.jmp_count));
        json_object_set_new(currentXref, "call_count", json_hex(i.second.call_count));

        JSON references = json_array();
        for(auto & record : i.second.references)
        {
            JSON currentRecord = json_object();
            json_object_set_new(currentRecord, "inst", json_string(record.second.inst));
            json_object_set_new(currentRecord, "addr", json_hex(record.second.addr));
            json_array_append_new(references, currentRecord);
        }

        json_object_set_new(currentXref, "references", references);

        json_array_append_new(jsonXrefs, currentXref);

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
    JSON value, record;
    json_array_foreach(jsonXrefs, i, value)
    {
        XrefInfo_t xrefinfo;

        // Copy module name
        const char* mod = json_string_value(json_object_get(value, "module"));

        if(mod && *mod && strlen(mod) < MAX_MODULE_SIZE)
            strcpy_s(xrefinfo.mod, mod);

        xrefinfo.address = (duint)json_hex_value(json_object_get(value, "address"));
        xrefinfo.jmp_count = (duint)json_hex_value(json_object_get(value, "jmp_count"));
        xrefinfo.call_count = (duint)json_hex_value(json_object_get(value, "call_count"));

        JSON jsonRecords = json_object_get(value, "references");
        // Load records
        json_array_foreach(jsonRecords, j, record)
        {
            XREF_RECORD xrefRecord;
            xrefRecord.addr = json_hex_value(json_object_get(record, "addr"));
            const char* inst = json_string_value(json_object_get(record, "inst"));
            strcpy_s(xrefRecord.inst, inst);
            xrefinfo.references.insert(std::make_pair(ModBaseFromName(xrefinfo.mod) + xrefRecord.addr, xrefRecord));
        }

        const duint key = ModHashFromName(xrefinfo.mod) + xrefinfo.address;
        references.insert(std::make_pair(key, xrefinfo));
    }



}

void XrefClear()
{
    EXCLUSIVE_ACQUIRE(LockCrossReferences);
    references.clear();
}
