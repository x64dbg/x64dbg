#pragma once

#include "_global.h"
#include <functional>

class ExpressionFunctions
{
public:
    using CBEXPRESSIONFUNCTIONINT = std::function<duint(int argc, const duint* argv, void* userdata)>;
    using CBEXPRESSIONFUNCTIONSTR = std::function<duint(int argc, const char** argv, void* userdata)>;

    static void Init();
    static bool RegisterInt(const String & name, int argc, const CBEXPRESSIONFUNCTIONINT & cbFunction, void* userdata = nullptr);
    static bool RegisterStr(const String & name, int argc, const CBEXPRESSIONFUNCTIONSTR & cbFunction, void* userdata = nullptr);
    static bool RegisterAlias(const String & name, const String & alias);
    static bool Unregister(const String & name);
    static bool CallInt(const String & name, std::vector<duint> & argv, duint & result);
    static bool CallStr(const String & name, std::vector<const char*> & argv, duint & result);
    static bool GetArgc(const String & name, int & argc, bool & strFunction);

private:
    struct Function
    {
        String name;
        int argc = 0;
        CBEXPRESSIONFUNCTIONINT cbFunctionInt;
        CBEXPRESSIONFUNCTIONSTR cbFunctionStr;
        void* userdata = nullptr;
        std::vector<String> aliases;
        bool strFunction = false;
    };

    static bool isValidName(const String & name);

    static std::unordered_map<String, Function> mFunctions;
};