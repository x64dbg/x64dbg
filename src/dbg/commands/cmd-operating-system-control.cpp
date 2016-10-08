#include "cmd-operating-system-control.h"
#include "variable.h"
#include "debugger.h"
#include "exception.h"
#include "value.h"

CMDRESULT cbGetPrivilegeState(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return STATUS_ERROR;
    DWORD returnLength;
    LUID luid;
    if(LookupPrivilegeValueW(nullptr, StringUtils::Utf8ToUtf16(argv[1]).c_str(), &luid) == 0)
    {
        varset("$result", (duint)0, false);
        return STATUS_CONTINUE;
    }
    Memory <TOKEN_PRIVILEGES*> Privileges(64 * 16 + 8, "_dbg_getprivilegestate");
    if(GetTokenInformation(hProcessToken, TokenPrivileges, Privileges(), 64 * 16 + 8, &returnLength) == 0)
    {
        if(returnLength > 4 * 1024 * 1024)
        {
            varset("$result", (duint)0, false);
            return STATUS_CONTINUE;
        }
        Privileges.realloc(returnLength, "_dbg_getprivilegestate");
        if(GetTokenInformation(hProcessToken, TokenPrivileges, Privileges(), returnLength, &returnLength) == 0)
            return STATUS_ERROR;
    }
    for(unsigned int i = 0; i < Privileges()->PrivilegeCount; i++)
    {
        if(4 + sizeof(LUID_AND_ATTRIBUTES) * i > returnLength)
            return STATUS_ERROR;
        if(memcmp(&Privileges()->Privileges[i].Luid, &luid, sizeof(LUID)) == 0)
        {
            varset("$result", (duint)(Privileges()->Privileges[i].Attributes + 1), false); // 2=enabled, 3=default, 1=disabled
            return STATUS_CONTINUE;
        }
    }
    varset("$result", (duint)0, false);
    return STATUS_CONTINUE;
}

CMDRESULT cbEnablePrivilege(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return STATUS_ERROR;
    LUID luid;
    if(LookupPrivilegeValueW(nullptr, StringUtils::Utf8ToUtf16(argv[1]).c_str(), &luid) == 0)
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not find the specified privilege: %s\n"), argv[1]);
        return STATUS_ERROR;
    }
    TOKEN_PRIVILEGES Privilege;
    Privilege.PrivilegeCount = 1;
    Privilege.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    Privilege.Privileges[0].Luid = luid;
    bool ret = AdjustTokenPrivileges(hProcessToken, FALSE, &Privilege, sizeof(TOKEN_PRIVILEGES), nullptr, nullptr) != NO_ERROR;
    return ret ? STATUS_CONTINUE : STATUS_ERROR;
}

CMDRESULT cbDisablePrivilege(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return STATUS_ERROR;
    LUID luid;
    if(LookupPrivilegeValueW(nullptr, StringUtils::Utf8ToUtf16(argv[1]).c_str(), &luid) == 0)
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not find the specified privilege: %s\n"), argv[1]);
        return STATUS_ERROR;
    }
    TOKEN_PRIVILEGES Privilege;
    Privilege.PrivilegeCount = 1;
    Privilege.Privileges[0].Attributes = 0;
    Privilege.Privileges[0].Luid = luid;
    bool ret = AdjustTokenPrivileges(hProcessToken, FALSE, &Privilege, sizeof(TOKEN_PRIVILEGES), nullptr, nullptr) != NO_ERROR;
    return ret ? STATUS_CONTINUE : STATUS_ERROR;
}

CMDRESULT cbHandleClose(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return STATUS_ERROR;
    duint handle;
    if(!valfromstring(argv[1], &handle, false))
        return STATUS_ERROR;
    if(!handle || !DuplicateHandle(fdProcessInfo->hProcess, HANDLE(handle), NULL, NULL, 0, FALSE, DUPLICATE_CLOSE_SOURCE))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "DuplicateHandle failed: %s\n"), ErrorCodeToName(GetLastError()).c_str());
        return STATUS_ERROR;
    }
#ifdef _WIN64
    dprintf(QT_TRANSLATE_NOOP("DBG", "Handle %llX closed!\n"), handle);
#else //x86
    dprintf(QT_TRANSLATE_NOOP("DBG", "Handle %X closed!\n"), handle);
#endif
    return STATUS_CONTINUE;
}