/**
@file exhandlerinfo.cpp

@brief ???
*/

#include "exhandlerinfo.h"
#include "memory.h"
#include "thread.h"
#include "value.h"
#include "debugger.h"

bool IsVistaOrLater()
{
    static bool vistaOrLater = []()
    {
        OSVERSIONINFOEXW osvi = { 0 };
        osvi.dwOSVersionInfoSize = sizeof(osvi);
        return GetVersionExW((LPOSVERSIONINFOW)&osvi) && osvi.dwMajorVersion > 5;
    }();
    return vistaOrLater;
}

bool Is19042OrLater()
{
    static bool is19042OrLater = []()
    {
        auto userSharedData = SharedUserData;
        return userSharedData->NtBuildNumber >= 19042;
    }();
    return is19042OrLater;
}

bool ExHandlerGetInfo(EX_HANDLER_TYPE Type, std::vector<duint> & Entries)
{
    Entries.clear();
    switch(Type)
    {
    case EX_HANDLER_SEH:
        return ExHandlerGetSEH(Entries);

    case EX_HANDLER_VEH:
        return ExHandlerGetVEH(Entries);

    case EX_HANDLER_VCH:
        return ExHandlerGetVCH(Entries, false);

    case EX_HANDLER_UNHANDLED:
        return ExHandlerGetUnhandled(Entries);
    }
    return false;
}

bool ExHandlerGetInfo(EX_HANDLER_TYPE Type, EX_HANDLER_INFO* Info)
{
    std::vector<duint> handlerEntries;
    if(!ExHandlerGetInfo(Type, handlerEntries))
    {
        Info->count = 0;
        Info->addresses = nullptr;
        return false;
    }

    // Convert vector to C-style array
    Info->count = (int)handlerEntries.size();
    Info->addresses = (duint*)BridgeAlloc(Info->count * sizeof(duint));

    memcpy(Info->addresses, handlerEntries.data(), Info->count * sizeof(duint));
    return true;
}

#define MAX_HANDLER_DEPTH 10

bool ExHandlerGetSEH(std::vector<duint> & Entries)
{
#ifdef _WIN64
    return false; // TODO: 64-bit
#endif
    static duint nextSEH = 0;
    NT_TIB tib;
    if(ThreadGetTib((duint)GetTEBLocation(hActiveThread), &tib))
    {
        EXCEPTION_REGISTRATION_RECORD sehr;
        duint addr_ExRegRecord = (duint)tib.ExceptionList;
        int MAX_DEPTH = MAX_HANDLER_DEPTH;
        while(addr_ExRegRecord != 0xFFFFFFFF && MAX_DEPTH)
        {
            Entries.push_back(addr_ExRegRecord);
            if(!MemRead(addr_ExRegRecord, &sehr, sizeof(EXCEPTION_REGISTRATION_RECORD)))
                break;
            addr_ExRegRecord = (duint)sehr.Next;
            MAX_DEPTH--;
        }
    }
    return true;
}

#pragma pack(8)
struct VEH_ENTRY_XP
{
    duint Flink;
    duint Blink;
    duint VectoredHandler;
};

bool ExHandlerGetVEH(std::vector<duint> & Entries)
{
    // Try the address for Windows XP first (or older)
    //
    // VECTORED_EXCEPTION_NODE RtlpCalloutEntryList;
    static duint addr_RtlpCalloutEntryList = 0;

#ifdef _WIN64
    auto symbol = "RtlpCalloutEntryList";
#else
    auto symbol = "_RtlpCalloutEntryList";
#endif
    if(addr_RtlpCalloutEntryList || valfromstring(symbol, &addr_RtlpCalloutEntryList))
    {
        // Read head entry
        auto list_head = addr_RtlpCalloutEntryList;
        duint cur_entry;

        if(!MemRead(list_head, &cur_entry, sizeof(cur_entry)))
            return false;
        auto count = 0;

        while(cur_entry != list_head && count++ < MAX_HANDLER_DEPTH)
        {
            VEH_ENTRY_XP entry;
            if(!MemRead(cur_entry, &entry, sizeof(entry)))
                return false;
            auto handler = entry.VectoredHandler;
            MemDecodePointer(&handler, false);
            Entries.push_back(handler);
            if(!MemRead(cur_entry, &cur_entry, sizeof(cur_entry)))
                return false;
        }
        return true;
    }

    // Otherwise try the Windows Vista or newer version
    return ExHandlerGetVCH(Entries, true);
}

#pragma pack(8)
struct VEH_ENTRY_VISTA
{
    duint Flink;
    duint Blink;
    duint PtrRefCount;
    duint VectoredHandler; // unclear when this changed
    duint VectoredHandler2;
};

bool ExHandlerGetVCH(std::vector<duint> & Entries, bool GetVEH)
{
    // VECTORED_HANDLER_LIST LdrpVectorHandlerList[2];
    static duint addr_LdrpVectorHandlerList = 0;
    duint addrInc = sizeof(duint); //Vista+ has an extra ULONG_PTR in front of the structure

#ifdef _WIN64
    auto symbol = "LdrpVectorHandlerList";
#else
    auto symbol = "_LdrpVectorHandlerList";
#endif
    if(!addr_LdrpVectorHandlerList && !valfromstring(symbol, &addr_LdrpVectorHandlerList))
        return false;

    // Increase array index when using continue handlers
    if(!GetVEH)
        addrInc += sizeof(duint) + sizeof(LIST_ENTRY); //Vista+ has an extra ULONG_PTR

    // Read head entry
    auto list_head = addr_LdrpVectorHandlerList + addrInc;
    duint cur_entry;

    if(!MemRead(list_head, &cur_entry, sizeof(cur_entry)))
        return false;
    auto count = 0;

    while(cur_entry != list_head && count++ < MAX_HANDLER_DEPTH)
    {
        VEH_ENTRY_VISTA entry;
        if(!MemRead(cur_entry, &entry, sizeof(entry)))
            return false;
        auto handler = entry.VectoredHandler;
        // At some point Windows updated the structure
        if(Is19042OrLater())
            handler = entry.VectoredHandler2;
        if(!MemDecodePointer(&handler, true))
            return false;
        Entries.push_back(handler);
        if(!MemRead(cur_entry, &cur_entry, sizeof(cur_entry)))
            return false;
    }
    return true;
}

bool ExHandlerGetUnhandled(std::vector<duint> & Entries)
{
    static duint addr_BasepCurrentTopLevelFilter = 0;

    auto symbol = ArchValue("_BasepCurrentTopLevelFilter", "BasepCurrentTopLevelFilter");
    if(addr_BasepCurrentTopLevelFilter || valfromstring(symbol, &addr_BasepCurrentTopLevelFilter))
    {
        // Read external pointer
        duint handlerValue = 0;

        if(!MemRead(addr_BasepCurrentTopLevelFilter, &handlerValue, sizeof(duint)))
            return false;

        // Decode with remote process cookie
        if(!MemDecodePointer(&handlerValue, IsVistaOrLater()))
            return false;

        Entries.push_back(handlerValue);
        return true;
    }

    return false;
}