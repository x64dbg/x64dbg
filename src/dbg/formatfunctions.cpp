#include "formatfunctions.h"
#include "threading.h"
#include "value.h"
#include "memory.h"
#include "exception.h"
#include "ntdll/ntdll.h"

std::unordered_map<String, FormatFunctions::Function> FormatFunctions::mFunctions;

static FORMATRESULT formatErrorMsg(HMODULE DLL, const String & errName, DWORD code, char* dest, size_t destCount)
{
    const NTSTATUS ErrorStatus = code;
    PMESSAGE_RESOURCE_ENTRY Entry;
    NTSTATUS Status = RtlFindMessage(DLL,
                                     LDR_FORMAT_MESSAGE_FROM_SYSTEM_MESSAGE_TABLE,
                                     MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                     ErrorStatus,
                                     &Entry);
    if(!NT_SUCCESS(Status))
    {
        if(destCount < errName.size() + 1)
            return FORMAT_BUFFER_TOO_SMALL;
        else
        {
            memcpy(dest, errName.c_str(), errName.size() + 1);
            return FORMAT_SUCCESS;
        }
    }

    if((Entry->Flags & MESSAGE_RESOURCE_UNICODE) != 0)
    {
        String UTF8Description = StringUtils::TrimRight(StringUtils::Utf16ToUtf8((const wchar_t*)Entry->Text));
        if(UTF8Description.size() + 3 + errName.size() > destCount)
            return FORMAT_BUFFER_TOO_SMALL;
        else
        {
            sprintf_s(dest, destCount, "%s: %s", errName.c_str(), UTF8Description.c_str());
            return FORMAT_SUCCESS;
        }
    }
    else
    {
        String UTF8Description = StringUtils::TrimRight(StringUtils::LocalCpToUtf8((const char*)Entry->Text));
        if(UTF8Description.size() + 3 + errName.size() > destCount)
            return FORMAT_BUFFER_TOO_SMALL;
        else
        {
            sprintf_s(dest, destCount, "%s: %s", errName.c_str(), UTF8Description.c_str());
            return FORMAT_SUCCESS;
        }
    }
}

void FormatFunctions::Init()
{
    Register("mem", [](char* dest, size_t destCount, int argc, char* argv[], duint addr, void* userdata)
    {
        duint size;
        if(argc < 2 || !valfromstring(argv[1], &size))
        {
            strcpy_s(dest, destCount, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Invalid argument...")));
            return FORMAT_ERROR_MESSAGE;
        }
        if(size > 1024 * 1024 * 10) //10MB max
        {
            strcpy_s(dest, destCount, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Too much data (10MB max)...")));
            return FORMAT_ERROR_MESSAGE;
        }
        std::vector<unsigned char> data(size);
        if(!MemRead(addr, data.data(), data.size()))
        {
            strcpy_s(dest, destCount, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Failed to read memory...")));
            return FORMAT_ERROR_MESSAGE;
        }
        auto result = StringUtils::ToHex(data.data(), data.size());
        if(result.size() > destCount)
            return FORMAT_BUFFER_TOO_SMALL;
        strcpy_s(dest, destCount, result.c_str());
        return FORMAT_SUCCESS;
    });

    Register("winerror", [](char* dest, size_t destCount, int argc, char* argv[], duint code, void* userdata)
    {
        std::vector<wchar_t> helpMessage(destCount);
        String errName = ErrorCodeToName((unsigned int)code);
#ifdef _WIN64
        if((code >> 32) != 0)  //Data in high part: not an error code
        {
            errName = StringUtils::sprintf("%p", code);
            if(destCount < errName.size() + 1)
                return FORMAT_BUFFER_TOO_SMALL;
            else
            {
                memcpy(dest, errName.c_str(), errName.size() + 1);
                return FORMAT_SUCCESS;
            }
        }
#endif //_WIN64
        if(errName.size() == 0)
            errName = StringUtils::sprintf("%08X", DWORD(code));

        return formatErrorMsg(GetModuleHandleW(L"kernel32.dll"), errName, code, dest, destCount);
    });

    Register("ntstatus", [](char* dest, size_t destCount, int argc, char* argv[], duint code, void* userdata)
    {
        std::vector<wchar_t> helpMessage(destCount);
        String errName = NtStatusCodeToName((unsigned int)code);
        if(errName.size() == 0)
            errName = StringUtils::sprintf("%08X", DWORD(code));

        return formatErrorMsg(GetModuleHandleW(L"ntdll.dll"), errName, code, dest, destCount);
    });
}

bool FormatFunctions::Register(const String & type, const CBFORMATFUNCTION & cbFunction, void* userdata)
{
    if(!isValidName(type))
        return false;
    EXCLUSIVE_ACQUIRE(LockFormatFunctions);
    if(mFunctions.count(type))
        return false;
    Function f;
    f.type = type;
    f.cbFunction = cbFunction;
    f.userdata = userdata;
    mFunctions[type] = f;
    return true;
}

bool FormatFunctions::RegisterAlias(const String & name, const String & alias)
{
    EXCLUSIVE_ACQUIRE(LockFormatFunctions);
    auto found = mFunctions.find(name);
    if(found == mFunctions.end())
        return false;
    if(!Register(alias, found->second.cbFunction, found->second.userdata))
        return false;
    found->second.aliases.push_back(alias);
    return true;
}

bool FormatFunctions::Unregister(const String & name)
{
    EXCLUSIVE_ACQUIRE(LockFormatFunctions);
    auto found = mFunctions.find(name);
    if(found == mFunctions.end())
        return false;
    auto aliases = found->second.aliases;
    mFunctions.erase(found);
    for(const auto & alias : found->second.aliases)
        Unregister(alias);
    return true;
}

bool FormatFunctions::Call(std::vector<char> & dest, const String & type, std::vector<String> & argv, duint value)
{
    SHARED_ACQUIRE(LockFormatFunctions);
    auto found = mFunctions.find(type);
    if(found == mFunctions.end())
        return false;

    std::vector<char*> argvn(argv.size());
    for(size_t i = 0; i < argv.size(); i++)
        argvn[i] = (char*)argv[i].c_str();

    const auto & f = found->second;
    dest.resize(512, '\0');
fuckthis:
    auto result = f.cbFunction(dest.data(), dest.size() - 1, int(argv.size()), argvn.data(), value, f.userdata);
    if(result == FORMAT_BUFFER_TOO_SMALL)
    {
        dest.resize(dest.size() * 2, '\0');
        goto fuckthis;
    }
    return result != FORMAT_ERROR;
}

bool FormatFunctions::isValidName(const String & name)
{
    if(!name.length())
        return false;
    if(!(name[0] == '_' || isalpha(name[0])))
        return false;
    for(const auto & ch : name)
        if(!(isalnum(ch) || ch == '_' || ch == '.'))
            return false;
    return true;
}
