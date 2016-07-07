#pragma once

#include "_global.h"

class ExpressionFunctions
{
public:
    typedef duint(*CBEXPRESSIONFUNCTION)(int argc, const duint* argv);

    static void Init();
    static bool Register(const String & name, int argc, CBEXPRESSIONFUNCTION cbFunction);
    static bool Unregister(const String & name);
    static bool Call(const String & name, const std::vector<duint> & argv, duint & result);
    static bool GetArgc(const String & name, int & argc);

private:
    struct Function
    {
        String name;
        int argc;
        CBEXPRESSIONFUNCTION cbFunction;
    };

    static bool isValidName(const String & name);

    static std::unordered_map<String, Function> mFunctions;
};