#include "stringformat.h"
#include "value.h"
#include "symbolinfo.h"
#include "module.h"
#include "disasm_fast.h"
#include "disasm_helper.h"
#include "formatfunctions.h"

enum class ValueType
{
    Unknown,
    SignedDecimal,
    UnsignedDecimal,
    Hex,
    Pointer,
    String,
    AddrInfo,
    Module,
    Instruction
};

static String printValue(FormatValueType value, ValueType type)
{
    duint valuint = 0;
    char string[MAX_STRING_SIZE] = "";
    String result = "???";
    if(valfromstring(value, &valuint))
    {
        switch(type)
        {
        case ValueType::Unknown:
            break;
#ifdef _WIN64
        case ValueType::SignedDecimal:
            result = StringUtils::sprintf("%lld", valuint);
            break;
        case ValueType::UnsignedDecimal:
            result = StringUtils::sprintf("%llu", valuint);
            break;
        case ValueType::Hex:
            result = StringUtils::sprintf("%llX", valuint);
            break;
#else //x86
        case ValueType::SignedDecimal:
            result = StringUtils::sprintf("%d", valuint);
            break;
        case ValueType::UnsignedDecimal:
            result = StringUtils::sprintf("%u", valuint);
            break;
        case ValueType::Hex:
            result = StringUtils::sprintf("%X", valuint);
            break;
#endif //_WIN64
        case ValueType::Pointer:
            result = StringUtils::sprintf("%p", valuint);
            break;
        case ValueType::String:
            if(disasmgetstringatwrapper(valuint, string, false))
                result = string;
            break;
        case ValueType::AddrInfo:
        {
            auto symbolic = SymGetSymbolicName(valuint);
            if(disasmgetstringatwrapper(valuint, string, false))
                result = string;
            else if(symbolic.length())
                result = symbolic;
            else
                result.clear();
        }
        break;
        case ValueType::Module:
        {
            char mod[MAX_MODULE_SIZE] = "";
            ModNameFromAddr(valuint, mod, true);
            result = mod;
        }
        break;
        case ValueType::Instruction:
        {
            BASIC_INSTRUCTION_INFO info;
            if(!disasmfast(valuint, &info, true))
                result = "???";
            else
                result = info.instruction;
        }
        break;
        default:
            break;
        }
    }
    return result;
}

static bool typeFromCh(char ch, ValueType & type)
{
    switch(ch)
    {
    case 'd':
        type = ValueType::SignedDecimal;
        break;
    case 'u':
        type = ValueType::UnsignedDecimal;
        break;
    case 'p':
        type = ValueType::Pointer;
        break;
    case 's':
        type = ValueType::String;
        break;
    case 'x':
        type = ValueType::Hex;
        break;
    case 'a':
        type = ValueType::AddrInfo;
        break;
    case 'm':
        type = ValueType::Module;
        break;
    case 'i':
        type = ValueType::Instruction;
        break;
    default: //invalid format
        return false;
    }
    return true;
}

static const char* getArgExpressionType(const String & formatString, ValueType & type, String & complexArgs)
{
    size_t toSkip = 0;
    type = ValueType::Hex;
    complexArgs.clear();
    if(formatString.size() > 2 && !isdigit(formatString[0]) && formatString[1] == ':') //simple type
    {
        if(!typeFromCh(formatString[0], type))
            return nullptr;
        toSkip = 2; //skip '?:'
    }
    else if(formatString.size() > 2 && formatString.find('@') != String::npos) //complex type
    {
        for(; toSkip < formatString.length(); toSkip++)
            if(formatString[toSkip] == '@')
            {
                toSkip++;
                break;
            }
        complexArgs = formatString.substr(0, toSkip - 1);
        if(complexArgs.length() == 1 && typeFromCh(complexArgs[0], type))
            complexArgs.clear();
    }
    return formatString.c_str() + toSkip;
}

static unsigned int getArgNumType(const String & formatString, ValueType & type)
{
    String complexArgs;
    auto expression = getArgExpressionType(formatString, type, complexArgs);
    unsigned int argnum = 0;
    if(!expression || sscanf_s(expression, "%u", &argnum) != 1)
        type = ValueType::Unknown;
    return argnum;
}

static String handleFormatString(const String & formatString, const FormatValueVector & values)
{
    auto type = ValueType::Unknown;
    auto argnum = getArgNumType(formatString, type);
    if(type != ValueType::Unknown && argnum < values.size())
        return printValue(values.at(argnum), type);
    return GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "[Formatting Error]"));
}

String stringformat(String format, const FormatValueVector & values)
{
    int len = (int)format.length();
    String output;
    String formatString;
    bool inFormatter = false;
    for(int i = 0; i < len; i++)
    {
        //handle escaped format sequences "{{" and "}}"
        if(format[i] == '{' && (i + 1 < len && format[i + 1] == '{'))
        {
            output += "{";
            i++;
            continue;
        }
        if(format[i] == '}' && (i + 1 < len && format[i + 1] == '}'))
        {
            output += "}";
            i++;
            continue;
        }
        //handle actual formatting
        if(format[i] == '{' && !inFormatter) //opening bracket
        {
            inFormatter = true;
            formatString.clear();
        }
        else if(format[i] == '}' && inFormatter) //closing bracket
        {
            inFormatter = false;
            if(formatString.length())
            {
                output += handleFormatString(formatString, values);
                formatString.clear();
            }
        }
        else if(inFormatter) //inside brackets
            formatString += format[i];
        else //outside brackets
            output += format[i];
    }
    if(inFormatter && formatString.size())
        output += handleFormatString(formatString, values);
    else if(inFormatter)
        output += "{";
    return output;
}

static String printComplexValue(FormatValueType value, const String & complexArgs)
{
    auto split = StringUtils::Split(complexArgs, ';');
    duint valuint;
    if(!split.empty() && valfromstring(value, &valuint))
    {
        std::vector<char> dest;
        if(FormatFunctions::Call(dest, split[0], split, valuint))
            return String(dest.data());
    }
    return GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "[Formatting Error]"));
}

static String handleFormatStringInline(const String & formatString)
{
    auto type = ValueType::Unknown;
    String complexArgs;
    auto value = getArgExpressionType(formatString, type, complexArgs);
    if(!complexArgs.empty())
        return printComplexValue(value, complexArgs);
    else if(value && *value)
        return printValue(value, type);
    return GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "[Formatting Error]"));
}

String stringformatinline(String format)
{
    int len = (int)format.length();
    String output;
    String formatString;
    bool inFormatter = false;
    for(int i = 0; i < len; i++)
    {
        //handle escaped format sequences "{{" and "}}"
        if(format[i] == '{' && (i + 1 < len && format[i + 1] == '{'))
        {
            output += "{";
            i++;
            continue;
        }
        if(format[i] == '}' && (i + 1 < len && format[i + 1] == '}'))
        {
            output += "}";
            i++;
            continue;
        }
        //handle actual formatting
        if(format[i] == '{' && !inFormatter) //opening bracket
        {
            inFormatter = true;
            formatString.clear();
        }
        else if(format[i] == '}' && inFormatter) //closing bracket
        {
            inFormatter = false;
            if(formatString.length())
            {
                output += handleFormatStringInline(formatString);
                formatString.clear();
            }
        }
        else if(inFormatter) //inside brackets
            formatString += format[i];
        else //outside brackets
            output += format[i];
    }
    if(inFormatter && formatString.size())
        output += handleFormatStringInline(formatString);
    else if(inFormatter)
        output += "{";
    return output;
}
