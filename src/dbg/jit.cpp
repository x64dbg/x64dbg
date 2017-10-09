#include "jit.h"

bool IsProcessElevated()
{
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID SecurityIdentifier;
    if(!AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &SecurityIdentifier))
        return 0;

    BOOL IsAdminMember;
    if(!CheckTokenMembership(NULL, SecurityIdentifier, &IsAdminMember))
        IsAdminMember = FALSE;

    FreeSid(SecurityIdentifier);
    return !!IsAdminMember;
}

static bool readwritejitkey(wchar_t* jit_key_value, DWORD* jit_key_vale_size, char* key, arch arch_in, arch* arch_out, readwritejitkey_error_t* error, bool write)
{
    DWORD key_flags;
    DWORD lRv;
    HKEY hKey;
    DWORD dwDisposition;

    if(error != NULL)
        *error = ERROR_RW;

    if(write)
    {
        if(!IsProcessElevated())
        {
            if(error != NULL)
                *error = ERROR_RW_NOTADMIN;
            return false;
        }
        key_flags = KEY_WRITE;
    }
    else
        key_flags = KEY_READ;

    if(arch_out != NULL)
    {
        if(arch_in != x64 && arch_in != x32)
        {
#ifdef _WIN64
            *arch_out = x64;
#else //x86
            *arch_out = x32;
#endif //_WIN64
        }
        else
            *arch_out = arch_in;
    }

    if(arch_in == x64)
    {
#ifndef _WIN64
        if(!IsWow64())
        {
            if(error != NULL)
                *error = ERROR_RW_NOTWOW64;
            return false;
        }
        key_flags |= KEY_WOW64_64KEY;
#endif //_WIN64
    }
    else if(arch_in == x32)
    {
#ifdef _WIN64
        key_flags |= KEY_WOW64_32KEY;
#endif
    }

    if(write)
    {
        lRv = RegCreateKeyExW(HKEY_LOCAL_MACHINE, JIT_REG_KEY, 0, NULL, REG_OPTION_NON_VOLATILE, key_flags, NULL, &hKey, &dwDisposition);
        if(lRv != ERROR_SUCCESS)
            return false;

        lRv = RegSetValueExW(hKey, StringUtils::Utf8ToUtf16(key).c_str(), 0, REG_SZ, (BYTE*)jit_key_value, (DWORD)(*jit_key_vale_size) + 1);
    }
    else
    {
        lRv = RegOpenKeyExW(HKEY_LOCAL_MACHINE, JIT_REG_KEY, 0, key_flags, &hKey);
        if(lRv != ERROR_SUCCESS)
        {
            if(error != NULL)
                *error = ERROR_RW_FILE_NOT_FOUND;
            return false;
        }

        lRv = RegQueryValueExW(hKey, StringUtils::Utf8ToUtf16(key).c_str(), 0, NULL, (LPBYTE)jit_key_value, jit_key_vale_size);
        if(lRv != ERROR_SUCCESS)
        {
            if(error != NULL)
                *error = ERROR_RW_FILE_NOT_FOUND;
        }
    }

    RegCloseKey(hKey);
    return (lRv == ERROR_SUCCESS);
}

bool dbggetjitauto(bool* auto_on, arch arch_in, arch* arch_out, readwritejitkey_error_t* rw_error_out)
{
    wchar_t jit_entry[4] = L"";
    DWORD jit_entry_size = sizeof(jit_entry) - 1;
    readwritejitkey_error_t rw_error;

    if(!readwritejitkey(jit_entry, &jit_entry_size, "Auto", arch_in, arch_out, &rw_error, false))
    {
        if(rw_error == ERROR_RW_FILE_NOT_FOUND)
        {
            if(rw_error_out != NULL)
                *rw_error_out = rw_error;
            return true;
        }
        return false;
    }
    if(_wcsicmp(jit_entry, L"1") == 0)
        *auto_on = true;
    else if(_wcsicmp(jit_entry, L"0") == 0)
        *auto_on = false;
    else
        return false;
    return true;
}

bool dbgsetjitauto(bool auto_on, arch arch_in, arch* arch_out, readwritejitkey_error_t* rw_error_out)
{
    DWORD auto_string_size = sizeof(L"1");
    readwritejitkey_error_t rw_error;
    if(!auto_on)
    {
        wchar_t jit_entry[4] = L"";
        DWORD jit_entry_size = sizeof(jit_entry) - 1;
        if(!readwritejitkey(jit_entry, &jit_entry_size, "Auto", arch_in, arch_out, &rw_error, false))
        {
            if(rw_error == ERROR_RW_FILE_NOT_FOUND)
                return true;
        }
    }
    if(!readwritejitkey((wchar_t*)(auto_on ? L"1" : L"0"), &auto_string_size, "Auto", arch_in, arch_out, &rw_error, true))
    {
        if(rw_error_out != NULL)
            *rw_error_out = rw_error;
        return false;
    }
    return true;
}

bool dbggetjit(char jit_entry[JIT_ENTRY_MAX_SIZE], arch arch_in, arch* arch_out, readwritejitkey_error_t* rw_error_out)
{
    wchar_t wszJitEntry[JIT_ENTRY_MAX_SIZE] = L"";
    DWORD jit_entry_size = JIT_ENTRY_MAX_SIZE * sizeof(wchar_t);
    readwritejitkey_error_t rw_error;
    if(!readwritejitkey(wszJitEntry, &jit_entry_size, "Debugger", arch_in, arch_out, &rw_error, false))
    {
        if(rw_error_out != NULL)
            *rw_error_out = rw_error;
        return false;
    }
    strcpy_s(jit_entry, JIT_ENTRY_MAX_SIZE, StringUtils::Utf16ToUtf8(wszJitEntry).c_str());
    return true;
}

bool dbggetdefjit(char* jit_entry)
{
    char path[JIT_ENTRY_DEF_SIZE];
    path[0] = '"';
    wchar_t wszPath[MAX_PATH] = L"";
    GetModuleFileNameW(GetModuleHandleW(NULL), wszPath, MAX_PATH);
    strcpy_s(&path[1], JIT_ENTRY_DEF_SIZE - 1, StringUtils::Utf16ToUtf8(wszPath).c_str());
    strcat_s(path, ATTACH_CMD_LINE);
    strcpy_s(jit_entry, JIT_ENTRY_DEF_SIZE, path);
    return true;
}

bool dbgsetjit(char* jit_cmd, arch arch_in, arch* arch_out, readwritejitkey_error_t* rw_error_out)
{
    DWORD jit_cmd_size = (DWORD)strlen(jit_cmd) * sizeof(wchar_t);
    readwritejitkey_error_t rw_error;
    if(!readwritejitkey((wchar_t*)StringUtils::Utf8ToUtf16(jit_cmd).c_str(), &jit_cmd_size, "Debugger", arch_in, arch_out, &rw_error, true))
    {
        if(rw_error_out != NULL)
            *rw_error_out = rw_error;
        return false;
    }
    return true;
}