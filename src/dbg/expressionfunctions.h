#pragma once

#include "_global.h"
#include <functional>

class ExpressionFunctions
{
public:
    using CBEXPRESSIONFUNCTION = std::function<duint(int argc, duint* argv, void* userdata)>;

    static void Init();
    static bool Register(const String & name, int argc, const CBEXPRESSIONFUNCTION & cbFunction, void* userdata = nullptr);
    static bool RegisterAlias(const String & name, const String & alias);
    static bool Unregister(const String & name);
    static bool Call(const String & name, std::vector<duint> & argv, duint & result);
    static bool GetArgc(const String & name, int & argc);

private:
    struct Function
    {
        String name;
        int argc = 0;
        CBEXPRESSIONFUNCTION cbFunction;
        void* userdata = nullptr;
        std::vector<String> aliases;
    };

    static bool isValidName(const String & name);

    static std::unordered_map<String, Function> mFunctions;
};