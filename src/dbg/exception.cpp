#include <unordered_map>
#include "exception.h"
#include "filehelper.h"
#include "value.h"
#include "console.h"

std::unordered_map<unsigned int, String> ExceptionNames;

bool ExceptionCodeInit(const String & exceptionFile)
{
    ExceptionNames.clear();
    std::vector<String> lines;
    if(!FileHelper::ReadAllLines(exceptionFile, lines))
        return false;
    auto parseLine = [](const String & line)
    {
        auto split = StringUtils::Split(line, ' ');
        if(int(split.size()) < 2)
        {
            dprintf("Invalid line: \"%s\"\n", line.c_str());
            return false;
        }
        duint code;
        if(!convertNumber(split[0].c_str(), code, 16))
        {
            dprintf("Failed to convert number \"%s\"\n", split[0].c_str());
            return false;
        }
        ExceptionNames.insert({ (unsigned int)code, split[1] });
        return true;
    };
    auto result = true;
    for(const auto & line : lines)
        if(!parseLine(line))
            result = false;
    return result;
}

String ExceptionCodeToName(unsigned int ExceptionCode)
{
    if(ExceptionNames.find(ExceptionCode) == ExceptionNames.end())
        return "";

    return ExceptionNames[ExceptionCode];
}