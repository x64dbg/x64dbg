#pragma once

#include "_global.h"
#include "_plugins.h"
#include <functional>

class FormatFunctions
{
public:
    using CBFORMATFUNCTION = std::function<FORMATRESULT(char* dest, size_t destCount, int argc, char* argv[], duint value, void* userdata)>;

    static void Init();
    static bool Register(const String & type, const CBFORMATFUNCTION & cbFunction, void* userdata = nullptr);
    static bool RegisterAlias(const String & type, const String & alias);
    static bool Unregister(const String & type);
    static bool Call(std::vector<char> & dest, const String & type, std::vector<String> & argv, duint value);

private:
    struct Function
    {
        String type;
        CBFORMATFUNCTION cbFunction;
        void* userdata;
        std::vector<String> aliases;
    };

    static bool isValidName(const String & name);

    static std::unordered_map<String, Function> mFunctions;
};