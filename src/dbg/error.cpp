#include <unordered_map>
#include "error.h"
#include "filehelper.h"
#include "value.h"
#include "console.h"

std::unordered_map<unsigned int, String> ErrorNames;

bool ErrorCodeInit(const String & errorFile)
{
    ErrorNames.clear();
    std::vector<String> lines;
    if(!FileHelper::ReadAllLines(errorFile, lines))
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
        if(!convertNumber(split[0].c_str(), code, 10))
        {
            dprintf("Failed to convert number \"%s\"\n", split[0].c_str());
            return false;
        }
        ErrorNames.insert({ (unsigned int)code, split[1] });
        return true;
    };
    auto result = true;
    for(const auto & line : lines)
        if(!parseLine(line))
            result = false;
    return result;
}

String ErrorCodeToName(unsigned int ErrorCode)
{
    if(ErrorNames.find(ErrorCode) == ErrorNames.end())
        return "";

    return ErrorNames[ErrorCode];
}