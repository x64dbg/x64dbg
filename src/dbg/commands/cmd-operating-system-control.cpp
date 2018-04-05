#include "cmd-operating-system-control.h"
#include "variable.h"
#include "debugger.h"
#include "exception.h"
#include "value.h"
#include "stringformat.h"

bool cbGetPrivilegeState(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    DWORD returnLength;
    LUID luid;
    if(LookupPrivilegeValueW(nullptr, StringUtils::Utf8ToUtf16(argv[1]).c_str(), &luid) == 0)
    {
        varset("$result", (duint)0, false);
        return true;
    }
    Memory <TOKEN_PRIVILEGES*> Privileges(64 * 16 + 8, "_dbg_getprivilegestate");
    if(GetTokenInformation(hProcessToken, TokenPrivileges, Privileges(), 64 * 16 + 8, &returnLength) == 0)
    {
        if(returnLength > 4 * 1024 * 1024)
        {
            varset("$result", (duint)0, false);
            return true;
        }
        Privileges.realloc(returnLength, "_dbg_getprivilegestate");
        if(GetTokenInformation(hProcessToken, TokenPrivileges, Privileges(), returnLength, &returnLength) == 0)
            return false;
    }
    for(unsigned int i = 0; i < Privileges()->PrivilegeCount; i++)
    {
        if(4 + sizeof(LUID_AND_ATTRIBUTES) * i > returnLength)
            return false;
        if(memcmp(&Privileges()->Privileges[i].Luid, &luid, sizeof(LUID)) == 0)
        {
            varset("$result", (duint)(Privileges()->Privileges[i].Attributes + 1), false); // 2=enabled, 3=default, 1=disabled
            return true;
        }
    }
    varset("$result", (duint)0, false);
    return true;
}

bool cbEnablePrivilege(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    LUID luid;
    if(LookupPrivilegeValueW(nullptr, StringUtils::Utf8ToUtf16(argv[1]).c_str(), &luid) == 0)
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not find the specified privilege: %s\n"), argv[1]);
        return false;
    }
    TOKEN_PRIVILEGES Privilege;
    Privilege.PrivilegeCount = 1;
    Privilege.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    Privilege.Privileges[0].Luid = luid;
    return AdjustTokenPrivileges(hProcessToken, FALSE, &Privilege, sizeof(TOKEN_PRIVILEGES), nullptr, nullptr) != NO_ERROR;
}

bool cbDisablePrivilege(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    LUID luid;
    if(LookupPrivilegeValueW(nullptr, StringUtils::Utf8ToUtf16(argv[1]).c_str(), &luid) == 0)
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not find the specified privilege: %s\n"), argv[1]);
        return false;
    }
    TOKEN_PRIVILEGES Privilege;
    Privilege.PrivilegeCount = 1;
    Privilege.Privileges[0].Attributes = 0;
    Privilege.Privileges[0].Luid = luid;
    bool ret = AdjustTokenPrivileges(hProcessToken, FALSE, &Privilege, sizeof(TOKEN_PRIVILEGES), nullptr, nullptr) != NO_ERROR;
    return ret ? true : false;
}

bool cbHandleClose(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    duint handle;
    if(!valfromstring(argv[1], &handle, false))
        return false;
    if(!handle || !DuplicateHandle(fdProcessInfo->hProcess, HANDLE(handle), NULL, NULL, 0, FALSE, DUPLICATE_CLOSE_SOURCE))
    {
        String error = stringformatinline(StringUtils::sprintf("{winerror@%d}", GetLastError()));
        dprintf(QT_TRANSLATE_NOOP("DBG", "DuplicateHandle failed: %s\n"), error.c_str());
        return false;
    }
#ifdef _WIN64
    dprintf(QT_TRANSLATE_NOOP("DBG", "Handle %llX closed!\n"), handle);
#else //x86
    dprintf(QT_TRANSLATE_NOOP("DBG", "Handle %X closed!\n"), handle);
#endif
    return true;
}

bool cbEnableWindow(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    duint handle;
    if(!valfromstring(argv[1], &handle, false))
        return false;

    if(!IsWindowEnabled((HWND)handle))
        EnableWindow((HWND)handle, TRUE);

    return true;
}

bool cbDisableWindow(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    duint handle;
    if(!valfromstring(argv[1], &handle, false))
        return false;

    if(IsWindowEnabled((HWND)handle))
        EnableWindow((HWND)handle, FALSE);

    return true;
}