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

template<class T> String printFloatValue(FormatValueType value)
{
    static_assert(std::is_same<T, double>::value || std::is_same<T, float>::value, "This function is used to print float and double values.");
    String result;
    char buf[16]; // a safe buffer with sufficient length to prevent buffer overflow while parsing
    memset(buf, 0, sizeof(buf));
    strcpy_s(buf, value); // copy value into buf
    _strlwr_s(buf); // convert "XMM" to "xmm"
    size_t offset = 0;
    bool bad = false;
    if(buf[1] == 'm' && buf[2] == 'm' && (buf[0] == 'x' || buf[0] == 'y')) // begins with /[xy]mm/
    {
        int index = 0; // the index of XMM/YMM register
        int bufptr = 0; // where is the character after the XMM register string
        if(buf[3] >= '0' && buf[3] <= '9' && buf[4] >= '0' && buf[4] <= '9')
        {
            index = (buf[3] - '0') * 10 + (buf[4] - '0'); // convert "10" to 10
            if(index >= ArchValue(8, 16)) // limit to available XMM registers (32bit: XMM0~XMM7, 64bit: XMM0~XMM15)
                bad = true;
            bufptr = 5;
        }
        else if(buf[3] >= '0' && buf[3] <= '9')
        {
            index = buf[3] - '0'; // convert "7" to 7
            if(index >= ArchValue(8, 16)) // limit to available XMM registers (32bit: XMM0~XMM7, 64bit: XMM0~XMM15)
                bad = true;
            bufptr = 4;
        }
        else
            bad = true;
        if(!bad)
        {
            if(buf[bufptr] == '\0')  // [xy]mm\d{1,2}
                offset = offsetof(REGDUMP, regcontext.XmmRegisters[index]);
            else if(std::is_same<T, double>() && buf[0] == 'x' && buf[bufptr] == 'h' && buf[bufptr + 1] == '\0')  // xmm\d{1,2}h
                offset = offsetof(REGDUMP, regcontext.XmmRegisters[index].High);
            else if(buf[bufptr] == '[')
            {
                if(buf[bufptr + 1] >= '0' && buf[bufptr + 1] <= '9' && buf[bufptr + 2] == ']' && buf[bufptr + 3] == '\0')  // [xy]mm\d{1,2}\[\d\]
                {
                    int item = buf[bufptr + 1] - '0';
                    if(buf[0] == 'x' && item >= 0 && item < 16 / sizeof(T)) // xmm
                        offset = offsetof(REGDUMP, regcontext.XmmRegisters[index]) + item * sizeof(T);
                    else if(buf[0] == 'y' && item >= 0 && item < 32 / sizeof(T)) // ymm
                        offset = offsetof(REGDUMP, regcontext.YmmRegisters[index]) + item * sizeof(T);
                    else
                        bad = true;
                }
                else
                    bad = true;
            }
            else
                bad = true;
        }
    }
    else
        bad = true; // TO DO: ST(...)
    REGDUMP registers;
    if(!bad) // prints an FPU register
    {
        if(DbgGetRegDumpEx(&registers, sizeof(registers)))
        {
            T* ptr = (T*)((char*)&registers + offset);
            std::stringstream wFloatingStr;
            wFloatingStr << std::setprecision(std::numeric_limits<T>::digits10) << *ptr;
            result = wFloatingStr.str();
        }
        else
            result = "???";
    }
    else // prints a memory pointer
    {
        T data;
        duint valuint = 0;
        if(valfromstring(value, &valuint) && DbgMemRead(valuint, &data, sizeof(data)))
        {
            std::stringstream wFloatingStr;
            wFloatingStr << std::setprecision(std::numeric_limits<T>::digits10) << data;
            result = wFloatingStr.str();
        }
        else
            result = "???";
    }
    return result;
}

static String printValue(FormatValueType value, StringValueType type)
{
    duint valuint = 0;
    char string[MAX_STRING_SIZE] = "";
    String result = "???";
    if(type == StringValueType::FloatingPointDouble)
    {
        result = printFloatValue<double>(value);
    }
    else if(type == StringValueType::FloatingPointSingle)
    {
        result = printFloatValue<float>(value);
    }
    else if(valfromstring(value, &valuint))
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
