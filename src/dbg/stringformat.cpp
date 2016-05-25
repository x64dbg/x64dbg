#include "stringformat.h"
#include "value.h"
#include "disasm_helper.h"

namespace ValueType
{
    enum ValueType
    {
        Unknown,
        SignedDecimal,
        UnsignedDecimal,
        Hex,
        LongHex,
        Pointer,
        String
    };
}

static String printValue(FormatValueType value, ValueType::ValueType type)
{
    duint valuint = 0;
    bool validval = valfromstring(value, &valuint);
    char result[deflen] = "???";
    switch(type)
    {
    case ValueType::Unknown:
        break;
    case ValueType::SignedDecimal:
        if(validval)
            sprintf_s(result, "%" fext "d", valuint);
        break;
    case ValueType::UnsignedDecimal:
        if(validval)
            sprintf_s(result, "%" fext "u", valuint);
        break;
    case ValueType::Hex:
        if(validval)
            sprintf_s(result, "%" fext "X", valuint);
        break;
    case ValueType::LongHex:
        if(validval)
            sprintf_s(result, fhex, valuint);
        break;
    case ValueType::Pointer:
        if(validval)
            sprintf_s(result, "0x" fhex, valuint);
        break;
    case ValueType::String:
        if(validval)
        {
            STRING_TYPE strtype;
            char string[512] = "";
            if(disasmgetstringat(valuint, &strtype, string, string, 500))
                strcpy_s(result, string);
        }
        break;
    }
    return result;
}

static const char* getArgExpressionType(const String & formatString, ValueType::ValueType & type)
{
    auto hasExplicitType = false;
    type = ValueType::Hex;
    if(formatString.size() > 2 && formatString[1] == ':')
    {
        hasExplicitType = true;
        switch(formatString[0])
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
            type = ValueType::LongHex;
            break;
        default:
            hasExplicitType = false;
            break;
        }
    }
    auto expression = formatString.c_str();
    if(hasExplicitType)
        expression += 2;
    return expression;
}

static unsigned int getArgNumType(const String & formatString, ValueType::ValueType & type)
{
    auto expression = getArgExpressionType(formatString, type);
    unsigned int argnum = 0;
    if(!expression || sscanf(expression, "%u", &argnum) != 1)
        type = ValueType::Unknown;
    return argnum;
}

static String handleFormatString(const String & formatString, const FormatValueVector & values)
{
    ValueType::ValueType type = ValueType::Unknown;
    unsigned int argnum = getArgNumType(formatString, type);
    if(argnum < values.size())
        return printValue(values.at(argnum), type);
    return "[Formatting Error]";
}

String stringformat(String format, const FormatValueVector & values)
{
    StringUtils::ReplaceAll(format, "\\n", "\n");
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
    return output;
}

static String handleFormatStringInline(const String & formatString)
{
    auto type = ValueType::Unknown;
    auto value = getArgExpressionType(formatString, type);
    if(value && *value)
        return printValue(value, type);
    return "[Formatting Error]";
}

String stringformatinline(String format)
{
    StringUtils::ReplaceAll(format, "\\n", "\n");
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
        if(format[i] == '{' && !inFormatter)  //opening bracket
        {
            inFormatter = true;
            formatString.clear();
        }
        else if(format[i] == '}' && inFormatter)  //closing bracket
        {
            inFormatter = false;
            if(formatString.length())
            {
                output += handleFormatStringInline(formatString);
                formatString.clear();
            }
        }
        else if(inFormatter)  //inside brackets
            formatString += format[i];
        else //outside brackets
            output += format[i];
    }
    if(inFormatter && formatString.size())
        output += handleFormatStringInline(formatString);
    return output;
}