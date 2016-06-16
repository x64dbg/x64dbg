#include "datainst_helper.h"
#include "encodemap.h"
#include "stringutils.h"
#include <capstone_wrapper.h>

std::unordered_map<ENCODETYPE, std::string> disasmMap;

std::unordered_map<std::string, ENCODETYPE> assembleMap;

void initDataInstMap()
{
    disasmMap.clear();
    assembleMap.clear();
#define INSTRUCTION_MAP(type, desc)  disasmMap[type] = desc; \
    assembleMap[desc] = type;

    INSTRUCTION_MAP(enc_byte, "byte")
    INSTRUCTION_MAP(enc_word, "word")
    INSTRUCTION_MAP(enc_dword, "dword")
    INSTRUCTION_MAP(enc_fword, "fword")
    INSTRUCTION_MAP(enc_qword, "qword")
    INSTRUCTION_MAP(enc_tbyte, "tbyte")
    INSTRUCTION_MAP(enc_oword, "oword")
    INSTRUCTION_MAP(enc_real4, "real4")
    INSTRUCTION_MAP(enc_real8, "real8")
    INSTRUCTION_MAP(enc_real10, "real10")
    INSTRUCTION_MAP(enc_mmword, "mmword")
    INSTRUCTION_MAP(enc_xmmword, "xmmword")
    INSTRUCTION_MAP(enc_ymmword, "ymmword")
    INSTRUCTION_MAP(enc_ascii, "ascii")
    INSTRUCTION_MAP(enc_unicode, "unicode")

#undef INSTRUCTION_MAP

#define INSTRUCTION_ASSEMBLE_MAP(type, desc)  assembleMap[desc] = type;
    INSTRUCTION_ASSEMBLE_MAP(enc_byte, "db")
    INSTRUCTION_ASSEMBLE_MAP(enc_word, "dw")
    INSTRUCTION_ASSEMBLE_MAP(enc_dword, "dd")
    INSTRUCTION_ASSEMBLE_MAP(enc_qword, "dq")

#undef INSTRUCTION_ASSEMBLE_MAP

}




String GetDataTypeString(void* buffer, duint size, ENCODETYPE type)
{

    switch(type)
    {
    case enc_byte:
        return StringUtils::ToIntegralString<unsigned char>(buffer);
    case enc_word:
        return StringUtils::ToIntegralString<unsigned short>(buffer);
    case enc_dword:
        return StringUtils::ToIntegralString<unsigned int>(buffer);
    case enc_fword:
        return StringUtils::ToHex((char*)buffer, 6, true);
    case enc_qword:
        return StringUtils::ToIntegralString<unsigned long long int>(buffer);
    case enc_tbyte:
        return StringUtils::ToHex((char*)buffer, 10, true);
    case enc_oword:
        return StringUtils::ToHex((char*)buffer, 16, true);
    case enc_mmword:
    case enc_xmmword:
    case enc_ymmword:
        return StringUtils::ToHex((char*)buffer, size, false);
    case enc_real4:
        return StringUtils::ToFloatingString<float>(buffer);
    case enc_real8:
        return StringUtils::ToFloatingString<double>(buffer);
    case enc_real10:
        return StringUtils::ToHex((char*)buffer, 10, true);
    //return ToLongDoubleString(buffer);
    case enc_ascii:
        return String((const char*)buffer, size);
    case enc_unicode:
        return StringUtils::Utf16ToUtf8(WString((const wchar_t*)buffer, size / sizeof(wchar_t)));
    default:
        return StringUtils::ToIntegralString<unsigned char>(buffer);
    }
}

String GetDataInstMnemonic(ENCODETYPE type)
{
    if(disasmMap.find(type) == disasmMap.end())
        type == enc_byte;
    if(disasmMap.find(type) == disasmMap.end())
        return "???";
    return disasmMap[type];
}

String GetDataInstString(void* buffer, duint size, ENCODETYPE type)
{
    return GetDataInstMnemonic(type) + " " + GetDataTypeString(buffer, size, type);
}
duint decodesimpledata(const unsigned char* buffer, ENCODETYPE type)
{
    switch(type)
    {
    case enc_byte:
        return *((unsigned char*)buffer);
    case enc_word:
        return *((unsigned short*)buffer);
    case enc_dword:
        return *((unsigned int*)buffer);
#ifdef _WIN64
    case enc_qword:
        return *((unsigned long long int*)buffer);
#endif // _WIN64
    }
    return 0;
}

struct DataInstruction
{
    ENCODETYPE type;
    String oprand;
};

bool parsedatainstruction(const char* instruction, DataInstruction & di)
{
    di.type = enc_unknown;
    di.oprand = "";
    String instStr = StringUtils::Trim(String(instruction));
    size_t pos = instStr.find_first_of(" \t");
    String opcode = instStr.substr(0, pos);
    std::transform(opcode.begin(), opcode.end(), opcode.begin(), ::tolower);
    if(assembleMap.find(opcode) == assembleMap.end())
        return false;
    di.type = assembleMap[opcode];
    pos = instStr.find_first_not_of(" \t", pos);
    if(pos == String::npos)
        return false;
    di.oprand = instStr.substr(pos);
}

bool isdatainstruction(const char* instruction)
{
    DataInstruction di;
    parsedatainstruction(instruction, di);

    return di.type != enc_unknown;
}

bool tryassembledata(duint addr, unsigned char* dest, int destsize, int* size, const char* instruction, char* error)
{
    DataInstruction di;
    if(!parsedatainstruction(instruction, di))
    {
        if(di.oprand == "")
            strcpy_s(error, MAX_ERROR_SIZE, "Missing oprand");
        return false;
    }

    duint retsize = 0;
    String buffer;
    try
    {
        switch(di.type)
        {
        case enc_byte:
        case enc_word:
        case enc_dword:
        case enc_fword:
        case enc_qword:
        case enc_tbyte:
        case enc_oword:
        {
            retsize = GetEncodeTypeSize(di.type);
            buffer = StringUtils::FromHex(di.oprand, retsize, true);
            break;
        }
        case enc_mmword:
        case enc_xmmword:
        case enc_ymmword:
        {
            retsize = GetEncodeTypeSize(di.type);
            buffer = StringUtils::FromHex(di.oprand, retsize, false);
            break;
        }
        case enc_real4:
        {
            retsize = 4;
            float f = std::stof(di.oprand);
            buffer = String((char*)&f, 4);
            break;
        }

        case enc_real8:
        {
            retsize = 8;
            double d = std::stod(di.oprand);
            buffer = String((char*)&d, 8);
            break;
        }

        case enc_real10:
            strcpy_s(error, MAX_ERROR_SIZE, "80bit extended float is not supported");
            return false; //80 bit float is not supported in MSVC, need to add other library
        case enc_ascii:
        {
            if(di.oprand.size() > destsize)
            {
                strcpy_s(error, MAX_ERROR_SIZE, "string too long");
                if(size)
                {
                    *size = di.oprand.size();  //return correct size
                    return dest == nullptr;
                }
                return false;
            }
            else
            {
                retsize = di.oprand.size();
                buffer = di.oprand;
            }
            break;
        }

        case enc_unicode:
        {
            WString unicode = StringUtils::Utf8ToUtf16(di.oprand);

            if(unicode.size()*sizeof(wchar_t) > destsize)
            {
                strcpy_s(error, MAX_ERROR_SIZE, "string too long");
                if(size)
                {
                    retsize = unicode.size() * 2; //return correct size
                    return dest == nullptr;
                }

                return false;
            }
            else
            {
                retsize = unicode.size() * 2;
                buffer = String((char*)unicode.c_str(), retsize);
            }
            break;
        }
        default:
            return false;
        }
    }
    catch(const std::exception & e)
    {
        strcpy_s(error, MAX_ERROR_SIZE, e.what());
        return false;
    }
    if(size)
        *size = retsize;

    if(dest)
        memcpy_s((char*)dest, retsize, buffer.c_str(), retsize);
    return true;
}

bool trydisasm(const unsigned char* buffer, duint addr, DISASM_INSTR* instr, duint codesize)
{
    ENCODETYPE type = EncodeMapGetType(addr, codesize);
    duint size = EncodeMapGetSize(addr, codesize);
    if(type == enc_unknown || type == enc_code)
        return false;
    memset(instr, 0, sizeof(DISASM_INSTR));
    instr->argcount = 1;
    instr->type = instr_normal;
    instr->arg[0].type = arg_memory;
    instr->arg[0].value = decodesimpledata(buffer, type);
    instr->instr_size = size;
    String str = GetDataInstString((void*)buffer, MAX_DISASM_BUFFER, type);
    strcpy_s(instr->instruction, str.c_str());
    return true;
}

bool trydisasmfast(const unsigned char* data, duint addr, BASIC_INSTRUCTION_INFO* basicinfo, duint codesize)
{
    ENCODETYPE type = EncodeMapGetType(addr, codesize);
    duint size = EncodeMapGetSize(addr, codesize);
    if(type == enc_unknown || type == enc_code)
        return false;
    memset(basicinfo, 0, sizeof(BASIC_INSTRUCTION_INFO));
    basicinfo->type = TYPE_VALUE;
    basicinfo->size = size;
    String str = GetDataInstString((void*)data, MAX_DISASM_BUFFER, type);
    strcpy_s(basicinfo->instruction, str.c_str());
    basicinfo->value.size = VALUE_SIZE(size);
    basicinfo->value.value = decodesimpledata(data, type);
    return true;
}
