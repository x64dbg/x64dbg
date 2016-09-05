#include <unordered_map>
#include "exception.h"
#include "filehelper.h"
#include "value.h"
#include "console.h"

static std::unordered_map<unsigned int, String> ExceptionNames;
static std::unordered_map<unsigned int, String> NtStatusNames;

bool UniversalCodeInit(const String & file, std::unordered_map<unsigned int, String> & names)
{
    names.clear();
    std::vector<String> lines;
    if(!FileHelper::ReadAllLines(file, lines))
        return false;
    auto parseLine = [&names](const String & line)
    {
        auto split = StringUtils::Split(line, ' ');
        if(int(split.size()) < 2)
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid line: \"%s\"\n"), line.c_str());
            return false;
        }
        duint code;
        if(!convertNumber(split[0].c_str(), code, 16))
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "Failed to convert number \"%s\"\n"), split[0].c_str());
            return false;
        }
        names.insert({ (unsigned int)code, split[1] });
        return true;
    };
    auto result = true;
    for(const auto & line : lines)
        if(!parseLine(line))
            result = false;
    return result;
}

bool ExceptionCodeInit(const String & exceptionFile)
{
    return UniversalCodeInit(exceptionFile, ExceptionNames);
}

String ExceptionCodeToName(unsigned int ExceptionCode)
{
    if(ExceptionNames.find(ExceptionCode) == ExceptionNames.end())
        return NtStatusCodeToName(ExceptionCode); //try NTSTATUS codes next

    return ExceptionNames[ExceptionCode];
}

bool NtStatusCodeInit(const String & ntStatusFile)
{
    return UniversalCodeInit(ntStatusFile, NtStatusNames);
}

String NtStatusCodeToName(unsigned NtStatusCode)
{
    if(NtStatusNames.find(NtStatusCode) == NtStatusNames.end())
        return "";

    return NtStatusNames[NtStatusCode];
}
