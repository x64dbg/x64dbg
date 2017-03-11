#include "formatfunctions.h"
#include "threading.h"
#include "value.h"
#include "memory.h"

std::unordered_map<String, FormatFunctions::Function> FormatFunctions::mFunctions;

void FormatFunctions::Init()
{
    Register("mem", [](char* dest, size_t destCount, int argc, char* argv[], duint addr, void* userdata)
    {
        duint size;
        if(argc < 2 || !valfromstring(argv[1], &size) || size > 128)
            return false;
        std::vector<unsigned char> data(size);
        if(!MemRead(addr, data.data(), data.size()))
            return false;
        strncpy_s(dest, destCount, StringUtils::ToHex(data.data(), data.size()).c_str(), _TRUNCATE);
        return true;
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
    return f.cbFunction(dest.data(), dest.size(), int(argv.size()), argvn.data(), value, f.userdata);
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
