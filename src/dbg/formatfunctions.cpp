#include "formatfunctions.h"
#include "threading.h"
#include "value.h"
#include "memory.h"
#include "exception.h"

std::unordered_map<String, FormatFunctions::Function> FormatFunctions::mFunctions;

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
        DWORD success = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, DWORD(code), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), helpMessage.data(), DWORD(destCount), NULL);
        if(success > 0)
        {
            String UTF8ErrorMessage = StringUtils::Utf16ToUtf8(helpMessage.data());
            if(destCount < errName.size() + 3 + UTF8ErrorMessage.size())
                return FORMAT_BUFFER_TOO_SMALL;
            else
            {
                sprintf_s(dest, destCount, "%s: %s", errName.c_str(), UTF8ErrorMessage.c_str());
                return FORMAT_SUCCESS;
            }
        }
        else
        {
            if(destCount < errName.size() + 1)
                return FORMAT_BUFFER_TOO_SMALL;
            else
            {
                memcpy(dest, errName.c_str(), errName.size() + 1);
                return FORMAT_SUCCESS;
            }
        }
    });

    Register("ntstatus", [](char* dest, size_t destCount, int argc, char* argv[], duint code, void* userdata)
    {
        std::vector<wchar_t> helpMessage(destCount);
        String errName = NtStatusCodeToName((unsigned int)code);
        if(errName.size() == 0)
            errName = StringUtils::sprintf("%08X", DWORD(code));
        DWORD success = FormatMessageW(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS, GetModuleHandleW(L"ntdll.dll"), DWORD(code), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), helpMessage.data(), DWORD(destCount), NULL);
        if(success > 0)
        {
            String UTF8ErrorMessage = StringUtils::Utf16ToUtf8(helpMessage.data());
            if(destCount < errName.size() + 3 + UTF8ErrorMessage.size())
                return FORMAT_BUFFER_TOO_SMALL;
            else
            {
                sprintf_s(dest, destCount, "%s: %s", errName.c_str(), UTF8ErrorMessage.c_str());
                return FORMAT_SUCCESS;
            }
        }
        else
        {
            if(destCount < errName.size() + 1)
                return FORMAT_BUFFER_TOO_SMALL;
            else
            {
                memcpy(dest, errName.c_str(), errName.size() + 1);
                return FORMAT_SUCCESS;
            }
        }
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
