#include "stringformat.h"
#include "value.h"
#include "symbolinfo.h"
#include "module.h"
#include "disasm_fast.h"
#include "disasm_helper.h"
#include "formatfunctions.h"

enum class StringValueType
{
    Unknown,
    SignedDecimal,
    UnsignedDecimal,
    Hex,
    Pointer,
    String,
    AddrInfo,
    Module,
    Instruction,
    FloatingPointSingle,
    FloatingPointDouble
};

static String printValue(FormatValueType value, StringValueType type)
{
    duint valuint = 0;
    char string[MAX_STRING_SIZE] = "";
    String result = "???";
    if(valfromstring(value, &valuint))
    {
        switch(type)
        {
        case StringValueType::Unknown:
            break;
#ifdef _WIN64
        case StringValueType::SignedDecimal:
            result = StringUtils::sprintf("%lld", valuint);
            break;
        case StringValueType::UnsignedDecimal:
            result = StringUtils::sprintf("%llu", valuint);
            break;
        case StringValueType::Hex:
            result = StringUtils::sprintf("%llX", valuint);
            break;
#else //x86
        case StringValueType::SignedDecimal:
            result = StringUtils::sprintf("%d", valuint);
            break;
        case StringValueType::UnsignedDecimal:
            result = StringUtils::sprintf("%u", valuint);
            break;
        case StringValueType::Hex:
            result = StringUtils::sprintf("%X", valuint);
            break;
#endif //_WIN64
        case StringValueType::Pointer:
            result = StringUtils::sprintf("%p", valuint);
            break;
        case StringValueType::String:
            if(disasmgetstringatwrapper(valuint, string, false))
                result = string;
            break;
        case StringValueType::AddrInfo:
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
        case StringValueType::Module:
        {
            char mod[MAX_MODULE_SIZE] = "";
            ModNameFromAddr(valuint, mod, true);
            result = mod;
        }
        break;
        case StringValueType::Instruction:
        {
            BASIC_INSTRUCTION_INFO info;
            if(!disasmfast(valuint, &info, true))
                result = "???";
            else
                result = info.instruction;
        }
        break;
        case StringValueType::FloatingPointSingle:
        {
            // 0~7 prints ST(0)-ST(7)
            if(valuint < 8)
            {
                // TO DO
                result = "???";
            }
            else
            {
                float data;
                if(DbgMemRead(valuint, &data, sizeof(data)))
                {
                    std::stringstream wFloatingStr;
                    wFloatingStr << std::setprecision(std::numeric_limits<float>::digits10) << data;
                    result = wFloatingStr.str();
                }
                else
                    result = "???";
            }
        }
        break;
        case StringValueType::FloatingPointDouble :
        {
            // 0~7 prints ST(0)-ST(7)
            if(valuint < 8)
            {
                // TO DO
                result = "???";
            }
            else
            {
                double data;
                if(DbgMemRead(valuint, &data, sizeof(data)))
                {
                    std::stringstream wFloatingStr;
                    wFloatingStr << std::setprecision(std::numeric_limits<double>::digits10) << data;
                    result = wFloatingStr.str();
                }
                else
                    result = "???";
            }
        }
        break;
        default:
            break;
        }
    }
    return result;
}

static bool typeFromCh(char ch, StringValueType & type)
{
    switch(ch)
    {
    case 'd':
        type = StringValueType::SignedDecimal;
        break;
    case 'u':
        type = StringValueType::UnsignedDecimal;
        break;
    case 'p':
        type = StringValueType::Pointer;
        break;
    case 's':
        type = StringValueType::String;
        break;
    case 'x':
        type = StringValueType::Hex;
        break;
    case 'a':
        type = StringValueType::AddrInfo;
        break;
    case 'm':
        type = StringValueType::Module;
        break;
    case 'i':
        type = StringValueType::Instruction;
        break;
    case 'f':
        type = StringValueType::FloatingPointSingle;
        break;
    case 'F':
        type = StringValueType::FloatingPointDouble;
        break;
    default: //invalid format
        return false;
    }
    return true;
}

static const char* getArgExpressionType(const String & formatString, StringValueType & type, String & complexArgs)
{
    size_t toSkip = 0;
    type = StringValueType::Hex;
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

static unsigned int getArgNumType(const String & formatString, StringValueType & type)
{
    String complexArgs;
    auto expression = getArgExpressionType(formatString, type, complexArgs);
    unsigned int argnum = 0;
    if(!expression || sscanf_s(expression, "%u", &argnum) != 1)
        type = StringValueType::Unknown;
    return argnum;
}

static String handleFormatString(const String & formatString, const FormatValueVector & values)
{
    auto type = StringValueType::Unknown;
    auto argnum = getArgNumType(formatString, type);
    if(type != StringValueType::Unknown && argnum < values.size())
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
    auto type = StringValueType::Unknown;
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
