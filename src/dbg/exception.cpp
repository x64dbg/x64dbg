#include <unordered_map>
#include <algorithm>
#include "exception.h"
#include "filehelper.h"
#include "value.h"
#include "console.h"

static std::unordered_map<unsigned int, String> ExceptionNames;
static std::unordered_map<unsigned int, String> NtStatusNames;
static std::unordered_map<unsigned int, String> ErrorNames;
static std::unordered_map<String, unsigned int> Constants;
static std::unordered_map<unsigned int, String> SyscallIndices;

static bool UniversalCodeInit(const String & file, std::unordered_map<unsigned int, String> & names, unsigned char radix)
{
    names.clear();
    std::vector<String> lines;
    if(!FileHelper::ReadAllLines(file, lines))
        return false;
    auto result = true;
    for(const auto & line : lines)
    {
        auto split = StringUtils::Split(line, ' ');
        if(split.size() < 2)
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid line in exception database: \"%s\"\n"), line.c_str());
            result = false;
            break;
        }
        duint code;
        if(!convertNumber(split[0].c_str(), code, radix))
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "Failed to convert number in exception database \"%s\"\n"), split[0].c_str());
            result = false;
            break;
        }
        names.insert({ (unsigned int)code, split[1] });
    }
    return result;
}

bool ErrorCodeInit(const String & errorFile)
{
    return UniversalCodeInit(errorFile, ErrorNames, 10);
}

bool ExceptionCodeInit(const String & exceptionFile)
{
    return UniversalCodeInit(exceptionFile, ExceptionNames, 16);
}

bool ConstantCodeInit(const String & constantFile)
{
    std::unordered_map<unsigned int, String> names;
    if(!UniversalCodeInit(constantFile, names, 0))
        return false;
    for(auto it : names)
        Constants.insert({ it.second, it.first });
    return true;
}

bool ConstantFromName(const String & name, duint & value)
{
    auto found = Constants.find(name);
    if(found == Constants.end())
        return false;
    value = found->second;
    return true;
}

std::vector<CONSTANTINFO> ConstantList()
{
    std::vector<CONSTANTINFO> result;
    for(auto it = Constants.begin(); it != Constants.end(); ++it)
    {
        CONSTANTINFO info;
        info.name = it->first.c_str();
        info.value = it->second;
        result.push_back(info);
    }
    std::sort(result.begin(), result.end(), [](const CONSTANTINFO & a, const CONSTANTINFO & b)
    {
        return strcmp(a.name, b.name) < 0;
    });
    return result;
}

static bool ExceptionDatabaseNameToCode(const std::unordered_map<unsigned int, String>* db, const char* Name, unsigned int* ErrorCode)
{
    for(const auto & i : *db)
    {
        if(i.second.compare(Name) == 0)
        {
            *ErrorCode = i.first;
            return true;
        }
    }
    return false;
}

bool ExceptionNameToCode(const char* Name, unsigned int* ErrorCode)
{
    if(!ExceptionDatabaseNameToCode(&ExceptionNames, Name, ErrorCode))
        return ExceptionDatabaseNameToCode(&NtStatusNames, Name, ErrorCode);
    return true;
}

bool NtStatusCodeInit(const String & ntStatusFile)
{
    return UniversalCodeInit(ntStatusFile, NtStatusNames, 16);
}

static const String emptyString("");

static const String & ExceptionDatabaseCodeToName(std::unordered_map<unsigned int, String>* db, unsigned int ErrorCode, bool* success)
{
    auto i = db->find(ErrorCode);
    if(i == db->end())
    {
        *success = false;
        return emptyString;
    }
    *success = true;
    return i->second;
}

const String & ExceptionCodeToName(unsigned int ExceptionCode)
{
    bool success;
    const String & name = ExceptionDatabaseCodeToName(&ExceptionNames, ExceptionCode, &success);
    if(!success)
        return ExceptionDatabaseCodeToName(&NtStatusNames, ExceptionCode, &success);
    else
        return name;
}

std::vector<CONSTANTINFO> ExceptionList()
{
    std::vector<CONSTANTINFO> result;
    for(auto it = ExceptionNames.begin(); it != ExceptionNames.end(); ++it)
    {
        CONSTANTINFO info;
        info.name = it->second.c_str();
        info.value = it->first;
        result.push_back(info);
    }
    for(auto it = NtStatusNames.begin(); it != NtStatusNames.end(); ++it)
    {
        CONSTANTINFO info;
        info.name = it->second.c_str();
        info.value = it->first;
        result.push_back(info);
    }
    return result;
}

const String & NtStatusCodeToName(unsigned int NtStatusCode)
{
    bool success;
    return ExceptionDatabaseCodeToName(&NtStatusNames, NtStatusCode, &success);
}

const String & ErrorCodeToName(unsigned int ErrorCode)
{
    bool success;
    return ExceptionDatabaseCodeToName(&ErrorNames, ErrorCode, &success);
}

std::vector<CONSTANTINFO> ErrorCodeList()
{
    std::vector<CONSTANTINFO> result;
    for(auto it = ErrorNames.begin(); it != ErrorNames.end(); ++it)
    {
        CONSTANTINFO info;
        info.name = it->second.c_str();
        info.value = it->first;
        result.push_back(info);
    }
    std::sort(result.begin(), result.end(), [](const CONSTANTINFO & a, const CONSTANTINFO & b)
    {
        return strcmp(a.name, b.name) < 0;
    });
    return result;
}

#include "threading.h"
#include "module.h"

bool SyscallInit()
{
    EXCLUSIVE_ACQUIRE(LockModules);
    auto retrieveSyscalls = [](const char* modname)
    {
        auto ntdll = GetModuleHandleA(modname);
        if(!ntdll)
            return false;
        char szModuleName[MAX_PATH];
        if(!GetModuleFileNameA(ntdll, szModuleName, _countof(szModuleName)))
            return false;
        if(!ModLoad((duint)ntdll, 1, szModuleName))
            return false;
        auto info = ModInfoFromAddr((duint)ntdll);
        if(info)
        {
            for(const MODEXPORT & export : info->exports)
            {
                if(strncmp(export.name.c_str(), "Nt", 2) != 0)
                    continue;
                auto exportData = (const unsigned char*)ModRvaToOffset(info->fileMapVA, info->headers, export.rva);
                if(!exportData)
                    continue;
                // https://github.com/mrexodia/TitanHide/blob/1c6ba9796e320f399f998b23fba2729122597e87/TitanHide/ntdll.cpp#L75
                DWORD index = -1;
                for(int i = 0; i < 32; i++)
                {
                    if(exportData[i] == 0xC2 || exportData[i] == 0xC3)   //RET
                    {
                        break;
                    }
                    if(exportData[i] == 0xB8)   //mov eax,X
                    {
                        index = *(DWORD*)(exportData + i + 1);
                        break;
                    }
                }
                if(index != -1)
                    SyscallIndices.emplace(index, export.name);
            }
        }
        else
        {
            return false;
        }
        return true;
    };
    // TODO: support windows < 10 for user32
    // See: https://github.com/x64dbg/ScyllaHide/blob/6817d32581b7a420322f34e36b1a1c8c3e4b434c/Scylla/Win32kSyscalls.h
    auto result = retrieveSyscalls("ntdll.dll") && retrieveSyscalls("win32u.dll");
    ModClear(false);
    return result;
}

const String & SyscallToName(unsigned int index)
{
    auto found = SyscallIndices.find(index);
    return found != SyscallIndices.end() ? found->second : emptyString;
}