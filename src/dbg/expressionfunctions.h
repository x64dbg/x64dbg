#pragma once

#include "_global.h"

class ExpressionFunctions
{
public:
    using CBEXPRESSIONFUNCTION             = std::function<duint(int argc, const duint* argv)>;
    using CBEXPRESSIONFUNCTIONWITHUSERDATA = std::function<duint(int argc, const duint* argv, void* user)>;

    static void Init();
    static bool Register(const String & name, int argc, CBEXPRESSIONFUNCTION cbFunction);
    static bool Register(const String & name, int argc, CBEXPRESSIONFUNCTIONWITHUSERDATA cbFunction, void* user);
    static bool Unregister(const String & name);
    static bool Call(const String & name, const std::vector<duint> & argv, duint & result);
    static bool GetArgc(const String & name, int & argc);

private:
    struct Function
    {
        CBEXPRESSIONFUNCTION cbFunction;
        CBEXPRESSIONFUNCTIONWITHUSERDATA cbFunctionWithUserData;

        String name;
        int argc;
        void* userdata;
        duint Invoke(int argc, const duint* argv) const;
    };

    static bool isValidName(const String & name);

    static std::unordered_map<String, Function> mFunctions;
};