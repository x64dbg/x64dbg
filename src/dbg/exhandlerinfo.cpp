/**
@file exhandlerinfo.cpp

@brief ???
*/

#include "exhandlerinfo.h"
#include "memory.h"
#include "disasm_helper.h"
#include "disasm_fast.h"
#include "_exports.h"
#include "module.h"
#include "thread.h"

bool ExHandlerGetInfo(EX_HANDLER_TYPE Type, EX_HANDLER_INFO* Info)
{
    bool ret = false;
    std::vector<duint> handlerEntries;

    switch(Type)
    {
    case EX_HANDLER_SEH:
        ret = ExHandlerGetSEH(handlerEntries);
        break;

    case EX_HANDLER_VEH:
        ret = ExHandlerGetVEH(handlerEntries);
        break;

    case EX_HANDLER_VCH:
        ret = ExHandlerGetVCH(handlerEntries, false);
        break;

    case EX_HANDLER_UNHANDLED:
        ret = ExHandlerGetUnhandled(handlerEntries);
        break;
    }

    // Check if a call failed
    if(!ret)
    {
        Info->count = 0;
        Info->addresses = nullptr;
        return false;
    }

    // Convert vector to C-style array
    Info->count = (int)handlerEntries.size();
    Info->addresses = (duint*)BridgeAlloc(Info->count * sizeof(duint));

    memcpy(Info->addresses, handlerEntries.data(), Info->count * sizeof(duint));
    return false;
}

bool ExHandlerGetSEH(std::vector<duint> & Entries)
{
    // TODO: 64-bit
    static duint nextSEH = 0;
    NT_TIB tib;
    if(ThreadGetTib((duint)GetTEBLocation(hActiveThread), &tib))
    {
        EXCEPTION_REGISTRATION_RECORD sehr;
        duint addr_ExRegRecord = (duint)tib.ExceptionList;
        while(addr_ExRegRecord != 0xFFFFFFFF)
        {
            Entries.push_back(addr_ExRegRecord);
            MemRead(addr_ExRegRecord , &sehr, sizeof(EXCEPTION_REGISTRATION_RECORD));
            addr_ExRegRecord = (duint)sehr.Next;
        }
    }
    return true;
}

bool ExHandlerGetVEH(std::vector<duint> & Entries)
{
    // Try the address for Windows XP first (or older)
    //
    // VECTORED_EXCEPTION_NODE RtlpCalloutEntryList;
    static duint addr_RtlpCalloutEntryList = 0;

    if(addr_RtlpCalloutEntryList || valfromstring("ntdll:RtlpCalloutEntryList", &addr_RtlpCalloutEntryList))
    {
        // Read header node
        VECTORED_EXCEPTION_NODE node;
        memset(&node, 0, sizeof(VECTORED_EXCEPTION_NODE));

        if(!MemRead(addr_RtlpCalloutEntryList, &node, sizeof(VECTORED_EXCEPTION_NODE)))
            return false;

        // Move to the next link
        duint listCurrent = (duint)node.ListEntry.Flink;
        duint listEnd = addr_RtlpCalloutEntryList;

        while(listCurrent && listCurrent != listEnd)
        {
            duint handler = (duint)node.handler;

            MemDecodePointer(&handler);
            Entries.push_back(handler);

            // Move to next element
            memset(&node, 0, sizeof(VECTORED_EXCEPTION_NODE));

            if(!MemRead(listCurrent, &node, sizeof(VECTORED_EXCEPTION_NODE)))
                break;

            listCurrent = (duint)node.ListEntry.Flink;
        }
    }

    // Otherwise try the Windows Vista or newer version
    return ExHandlerGetVCH(Entries, true);
}

bool ExHandlerGetVCH(std::vector<duint> & Entries, bool UseVEH)
{
    // VECTORED_HANDLER_LIST LdrpVectorHandlerList[2];
    static duint addr_LdrpVectorHandlerList = 0;

    if(!addr_LdrpVectorHandlerList && !valfromstring("ntdll:LdrpVectorHandlerList", &addr_LdrpVectorHandlerList))
        return false;

    // Increase array index when using continue handlers
    if(!UseVEH)
        addr_LdrpVectorHandlerList += (1 * sizeof(VECTORED_HANDLER_LIST));

    // Read head entry
    VECTORED_HANDLER_LIST list;
    memset(&list, 0, sizeof(VECTORED_HANDLER_LIST));

    if(!MemRead(addr_LdrpVectorHandlerList, &list, sizeof(VECTORED_HANDLER_LIST)))
        return false;

    // Sub-entries in list
    duint listCurrent = (duint)list.Next;
    duint listEnd = addr_LdrpVectorHandlerList;

    while(listCurrent && listCurrent != listEnd)
    {
        duint handler = (duint)list.VectoredHandler;

        MemDecodePointer(&handler);
        Entries.push_back(handler);

        // Move to next element
        memset(&list, 0, sizeof(VECTORED_HANDLER_LIST));

        if(!MemRead(listCurrent, &list, sizeof(VECTORED_HANDLER_LIST)))
            break;

        listCurrent = (duint)list.Next;
    }

    return true;
}

bool ExHandlerGetUnhandled(std::vector<duint> & Entries)
{
    // Try the address for Windows Vista+
    static duint addr_BasepCurrentTopLevelFilter = 0;

    if(addr_BasepCurrentTopLevelFilter || valfromstring("kernelbase:BasepCurrentTopLevelFilter", &addr_BasepCurrentTopLevelFilter))
    {
        // Read external pointer
        duint handlerValue = 0;

        if(!MemRead(addr_BasepCurrentTopLevelFilter, &handlerValue, sizeof(duint)))
            return false;

        // Decode with remote process cookie
        if(!MemDecodePointer(&handlerValue))
            return false;

        Entries.push_back(handlerValue);
        return true;
    }

    return false;
}