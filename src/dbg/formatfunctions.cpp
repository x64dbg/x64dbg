#include "formatfunctions.h"
#include "threading.h"
#include "value.h"
#include "memory.h"
#include "exception.h"
#include "ntdll/ntdll.h"
#include "disasm_fast.h"

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

template<class Char, size_t DefaultSize = 0>
static FORMATRESULT memoryFormatter(char* dest, size_t destCount, int argc, char* argv[], duint addr, const std::function<String(std::vector<Char>&)> & format)
{
    duint size = DefaultSize;
    if(argc > 1 && !valfromstring(argv[1], &size))
    {
        strcpy_s(dest, destCount, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Invalid argument...")));
        return FORMAT_ERROR_MESSAGE;
    }
    if(size == 0)
    {
        strcpy_s(dest, destCount, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Not enough arguments...")));
        return FORMAT_ERROR_MESSAGE;
    }
    if(size > 1024 * 1024 * 10) //10MB max
    {
        strcpy_s(dest, destCount, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Too much data (10MB max)...")));
        return FORMAT_ERROR_MESSAGE;
    }
    std::vector<Char> data(size);
    duint read = 0;
    if(!MemRead(addr, data.data(), size * sizeof(Char), &read) && read == 0)
    {
        strcpy_s(dest, destCount, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Failed to read memory...")));
        return FORMAT_ERROR_MESSAGE;
    }
    data.resize(read);
    auto result = format(data);
    if(result.size() > destCount)
        return FORMAT_BUFFER_TOO_SMALL;
    strcpy_s(dest, destCount, result.c_str());
    return FORMAT_SUCCESS;
}

static FORMATRESULT formatcpy_s(char* dest, size_t destCount, const char* source)
{
    switch(strncpy_s(dest, destCount, source, _TRUNCATE))
    {
    case 0:
        return FORMAT_SUCCESS;
    case ERANGE:
    case STRUNCATE:
        return FORMAT_BUFFER_TOO_SMALL;
    default:
        return FORMAT_ERROR;
    }
}

void FormatFunctions::Init()
{
    Register("mem", [](char* dest, size_t destCount, int argc, char* argv[], duint addr, void* userdata)
    {
        return memoryFormatter<unsigned char>(dest, destCount, argc, argv, addr, [](std::vector<unsigned char> & data)
        {
            return StringUtils::ToHex(data.data(), data.size());
        });
    });

    Register("ascii", [](char* dest, size_t destCount, int argc, char* argv[], duint addr, void* userdata)
    {
        return memoryFormatter<unsigned char, 512>(dest, destCount, argc, argv, addr, [](std::vector<unsigned char> & data)
        {
            String result;
            result.reserve(data.size());
            for(auto & ch : data)
            {
                if(isprint(ch))
                    result.push_back(char(ch));
                else
                    result.push_back('?');
            }
            return result;
        });
    });

    Register("ansi", [](char* dest, size_t destCount, int argc, char* argv[], duint addr, void* userdata)
    {
        return memoryFormatter<char, 512>(dest, destCount, argc, argv, addr, [](std::vector<char> & data)
        {
            if(data.empty() || data.back() != '\0')
                data.push_back('\0');
            return StringUtils::LocalCpToUtf8(data.data());
        });
    });

    Register("utf8", [](char* dest, size_t destCount, int argc, char* argv[], duint addr, void* userdata)
    {
        return memoryFormatter<char, 512>(dest, destCount, argc, argv, addr, [](std::vector<char> & data)
        {
            if(data.empty() || data.back() != '\0')
                data.push_back('\0');
            return String(data.data());
        });
    });

    Register("utf16", [](char* dest, size_t destCount, int argc, char* argv[], duint addr, void* userdata)
    {
        return memoryFormatter<wchar_t, 512>(dest, destCount, argc, argv, addr, [](std::vector<wchar_t> & data)
        {
            if(data.empty() || data.back() != L'\0')
                data.push_back(L'\0');
            return StringUtils::Utf16ToUtf8(data.data());
        });
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

        return formatErrorMsg(GetModuleHandleW(L"kernel32.dll"), errName, DWORD(code), dest, destCount);
    });

    Register("ntstatus", [](char* dest, size_t destCount, int argc, char* argv[], duint code, void* userdata)
    {
        std::vector<wchar_t> helpMessage(destCount);
        String errName = NtStatusCodeToName((unsigned int)code);
        if(errName.size() == 0)
            errName = StringUtils::sprintf("%08X", DWORD(code));

        return formatErrorMsg(GetModuleHandleW(L"ntdll.dll"), errName, DWORD(code), dest, destCount);
    });

    Register("disasm", [](char* dest, size_t destCount, int argc, char* argv[], duint addr, void* userdata)
    {
        const char* result = nullptr;
        BASIC_INSTRUCTION_INFO info;
        if(!disasmfast(addr, &info, true))
            result = "???";
        else
            result = info.instruction;
        return formatcpy_s(dest, destCount, result);
    });

    Register("modname", [](char* dest, size_t destCount, int argc, char* argv[], duint addr, void* userdata)
    {
        char mod[MAX_MODULE_SIZE] = "";
        if(!ModNameFromAddr(addr, mod, true))
            return FORMAT_ERROR;
        return formatcpy_s(dest, destCount, mod);
    });

    Register("bswap", [](char* dest, size_t destCount, int argc, char* argv[], duint addr, void* userdata)
    {
        duint size = sizeof(duint);
        if(argc > 1 && !valfromstring(argv[1], &size))
        {
            strcpy_s(dest, destCount, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Invalid argument...")));
            return FORMAT_ERROR_MESSAGE;
        }
        if(size > sizeof(duint) || size == 0)
        {
            strcpy_s(dest, destCount, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Invalid size...")));
            return FORMAT_ERROR_MESSAGE;
        }
        auto data = (unsigned char*)&addr;
        String result;
        for(duint i = 0; i < size; i++)
            result += StringUtils::sprintf("%02X", data[i]);
        return formatcpy_s(dest, destCount, result.c_str());
    });

    Register("comment", [](char* dest, size_t destCount, int argc, char* argv[], duint addr, void* userdata)
    {
        char comment[MAX_COMMENT_SIZE] = "";
        if(DbgGetCommentAt(addr, comment))
        {
            if(comment[0] == '\1') //automatic comment
                return formatcpy_s(dest, destCount, comment + 1);
            else
                return formatcpy_s(dest, destCount, comment);
        }
        return FORMAT_ERROR;
    });

    Register("label", [](char* dest, size_t destCount, int argc, char* argv[], duint addr, void* userdata)
    {
        char label[MAX_LABEL_SIZE] = "";
        if(DbgGetLabelAt(addr, SEG_DEFAULT, label))
            return formatcpy_s(dest, destCount, label);
        return FORMAT_ERROR;
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
