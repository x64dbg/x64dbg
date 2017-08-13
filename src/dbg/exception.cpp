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
