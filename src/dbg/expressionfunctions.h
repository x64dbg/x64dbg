#pragma once

#include "_global.h"
#include "_plugins.h"
#include <functional>


class ExpressionFunctions
{
public:
    // TODO: also register the argument types
    using CBEXPRESSIONFUNCTION = std::function<bool(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata)>;

    static void Init();
    static bool Register(const String & name, const ValueType & returnType, const std::vector<ValueType> & argTypes, const CBEXPRESSIONFUNCTION & cbFunction, void* userdata = nullptr);
    static bool RegisterAlias(const String & name, const String & alias);
    static bool Unregister(const String & name);
    static bool Call(const String & name, ExpressionValue & result, std::vector<ExpressionValue> & argv);
    static bool GetType(const String & name, ValueType & returnType, std::vector<ValueType> & argTypes);

private:
    struct Function
    {
        String name;
        ValueType returnType;
        std::vector<ValueType> argTypes;
        CBEXPRESSIONFUNCTION cbFunction;
        void* userdata = nullptr;
        std::vector<String> aliases;
    };

    static bool isValidName(const String & name);

    static std::unordered_map<String, Function> mFunctions;
};