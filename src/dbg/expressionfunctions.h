#pragma once

#include "_global.h"
#include <functional>

typedef enum
{
    ValueTypeNumber,
    ValueTypeString,
} ValueType;

typedef struct
{
    const char* ptr;
    bool isOwner;
} StringValue;

typedef struct
{
    ValueType type;
    duint number;
    StringValue string;
} ExpressionValue;

class ExpressionFunctions
{
public:
    // TODO: also register the argument types
    using CBEXPRESSIONFUNCTION = std::function<bool(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata)>;

    static void Init();
    static bool Register(const String & name, int argc, const CBEXPRESSIONFUNCTION & cbFunction, void* userdata = nullptr);
    static bool RegisterAlias(const String & name, const String & alias);
    static bool Unregister(const String & name);
    static bool Call(const String & name, ExpressionValue & result, std::vector<ExpressionValue> & argv);
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