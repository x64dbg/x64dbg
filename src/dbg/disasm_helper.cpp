/**
 @file disasm_helper.cpp

 @brief Implements the disasm helper class.
 */

#include "disasm_helper.h"
#include "thread.h"
#include "console.h"
#include "memory.h"
#include "debugger.h"
#include "value.h"
#include "encodemap.h"
#include <zydis_wrapper.h>
#include "datainst_helper.h"

duint disasmback(unsigned char* data, duint base, duint size, duint ip, int n)
{
    int i;
    duint abuf[131], addr, back, cmdsize;
    memset(abuf, 0, sizeof(abuf));
    unsigned char* pdata;

    // Reset Disasm Structure
    Zydis cp;

    // Check if the pointer is not null
    if(data == NULL)
        return 0;

    // Round the number of back instructions to 127
    if(n < 0)
        n = 0;
    else if(n > 127)
        n = 127;

    // Check if the instruction pointer ip is not outside the memory range
    if(ip >= size)
        ip = size - 1;

    // Obvious answer
    if(n == 0)
        return ip;

    if(ip < (duint)n)
        return ip;

    back = MAX_DISASM_BUFFER * (n + 3); // Instruction length limited to 16

    if(ip < back)
        back = ip;

    addr = ip - back;

    pdata = data + addr;

    for(i = 0; addr < ip; i++)
    {
        abuf[i % 128] = addr;

        if(!cp.Disassemble(0, pdata, (int)size))
            cmdsize = 1;
        else
            cmdsize = cp.Size();

        pdata += cmdsize;
        addr += cmdsize;
        back -= cmdsize;
        size -= cmdsize;
    }

    if(i < n)
        return abuf[0];
    else
        return abuf[(i - n + 128) % 128];
}

duint disasmnext(unsigned char* data, duint base, duint size, duint ip, int n)
{
    int i;
    duint cmdsize;
    unsigned char* pdata;

    // Reset Disasm Structure
    Zydis cp;

    if(data == NULL)
        return 0;

    if(ip >= size)
        ip = size - 1;

    if(n <= 0)
        return ip;

    pdata = data + ip;
    size -= ip;

    for(i = 0; i < n && size > 0; i++)
    {
        if(!cp.Disassemble(0, pdata, (int)size))
            cmdsize = 1;
        else
            cmdsize = cp.Size();

        pdata += cmdsize;
        ip += cmdsize;
        size -= cmdsize;
    }

    return ip;
}

static void HandleZydisOperand(Zydis & cp, int opindex, DISASM_ARG* arg, bool getregs)
{
    auto value = cp.ResolveOpValue(opindex, [&cp, getregs](ZydisRegister reg)
    {
        auto regName = getregs ? cp.RegName(reg) : nullptr;
        return regName ? getregister(nullptr, regName) : 0; //TODO: temporary needs enums + caching
    });
    const auto & op = cp[opindex];
    arg->segment = SEG_DEFAULT;
    auto opText = cp.OperandText(opindex);
    StringUtils::ReplaceAll(opText, "0x", "");
    strcpy_s(arg->mnemonic, opText.c_str());
    switch(op.type)
    {
    case ZYDIS_OPERAND_TYPE_REGISTER:
    {
        arg->type = arg_normal;
        arg->value = value;
    }
    break;

    case ZYDIS_OPERAND_TYPE_IMMEDIATE:
    {
        arg->type = arg_normal;
        arg->constant = arg->value = value;
    }
    break;

    case ZYDIS_OPERAND_TYPE_MEMORY:
    {
        arg->type = arg_memory;
        const auto & mem = op.mem;
        if(mem.base == ZYDIS_REGISTER_RIP) //rip-relative
            arg->constant = cp.Address() + duint(mem.disp.value) + cp.Size();
        else
            arg->constant = duint(mem.disp.value);
        if(mem.segment == ArchValue(ZYDIS_REGISTER_FS, ZYDIS_REGISTER_GS))
        {
            arg->segment = ArchValue(SEG_FS, SEG_GS);
            value += ThreadGetLocalBase(ThreadGetId(hActiveThread));
        }
        else
        {
            switch(mem.segment)
            {
            case ZYDIS_REGISTER_CS:
                arg->segment = SEG_CS;
                break;
            case ZYDIS_REGISTER_DS:
                arg->segment = SEG_DS;
                break;
            case ZYDIS_REGISTER_ES:
                arg->segment = SEG_ES;
                break;
            case ZYDIS_REGISTER_FS:
                arg->segment = SEG_FS;
                break;
            case ZYDIS_REGISTER_GS:
                arg->segment = SEG_GS;
                break;
            case ZYDIS_REGISTER_SS:
                arg->segment = SEG_SS;
                break;
            }
        }
        arg->value = value;
        if(DbgMemIsValidReadPtr(value))
        {
            switch(op.size)
            {
            case 8:
                MemRead(value, (unsigned char*)&arg->memvalue, 1);
                break;
            case 16:
                MemRead(value, (unsigned char*)&arg->memvalue, 2);
                break;
            case 32:
                MemRead(value, (unsigned char*)&arg->memvalue, 4);
                break;
#ifdef _WIN64
            case 48:
                MemRead(value, (unsigned char*)&arg->memvalue, 6);
                break;
            case 64:
                MemRead(value, (unsigned char*)&arg->memvalue, 8);
                break;
            default:
                //TODO: not supported
                break;
#endif //_WIN64
            }
        }
    }
    break;

    case ZYDIS_OPERAND_TYPE_POINTER:
        arg->type = arg_normal;
        arg->value = op.ptr.offset;
        break;

    default:
        break;
    }
}

void disasmget(Zydis & cp, unsigned char* buffer, duint addr, DISASM_INSTR* instr, bool getregs)
{
    memset(instr, 0, sizeof(DISASM_INSTR));
    cp.Disassemble(addr, buffer, MAX_DISASM_BUFFER);
    if(trydisasm(buffer, addr, instr, cp.Success() ? cp.Size() : 1))
        return;
    if(!cp.Success())
    {
        strcpy_s(instr->instruction, "???");
        instr->instr_size = 1;
        instr->type = instr_normal;
        instr->argcount = 0;
        return;
    }
    auto cpInstr = cp.GetInstr();
    strncpy_s(instr->instruction, cp.InstructionText().c_str(), _TRUNCATE);
    instr->instr_size = cpInstr->length;
    if(cp.IsBranchType(Zydis::BTJmp | Zydis::BTLoop | Zydis::BTRet | Zydis::BTCall))
        instr->type = instr_branch;
    else if(strstr(instr->instruction, "sp") || strstr(instr->instruction, "bp"))
        instr->type = instr_stack;
    else
        instr->type = instr_normal;
    instr->argcount = cp.OpCount() <= 3 ? cp.OpCount() : 3;
    for(int i = 0; i < instr->argcount; i++)
        HandleZydisOperand(cp, i, &instr->arg[i], getregs);
}

void disasmget(Zydis & cp, duint addr, DISASM_INSTR* instr, bool getregs)
{
    if(!DbgIsDebugging())
    {
        if(instr)
            instr->argcount = 0;
        return;
    }
    unsigned char buffer[MAX_DISASM_BUFFER] = "";
    if(MemRead(addr, buffer, sizeof(buffer)))
        disasmget(cp, buffer, addr, instr, getregs);
    else
        memset(instr, 0, sizeof(DISASM_INSTR)); // Buffer overflow
}

void disasmget(unsigned char* buffer, duint addr, DISASM_INSTR* instr, bool getregs)
{
    Zydis cp;
    disasmget(cp, buffer, addr, instr, getregs);
}

void disasmget(duint addr, DISASM_INSTR* instr, bool getregs)
{
    if(!DbgIsDebugging())
    {
        if(instr)
            instr->argcount = 0;
        return;
    }
    unsigned char buffer[MAX_DISASM_BUFFER] = "";
    if(MemRead(addr, buffer, sizeof(buffer)))
        disasmget(buffer, addr, instr, getregs);
    else
        memset(instr, 0, sizeof(DISASM_INSTR)); // Buffer overflow
}

bool isunicodestring(const WString & data)
{
    if(data.size() < 2)
        return false;
    for(auto & i : data)
    {
        // No specials
        if(i >= 0xFFF0)
            return false;
        // No ANSI control chars
        if(i < 0x80 && !isprint(i) && !isspace(i))
            return false;
        // No C1 control chars
        if(i >= 0x80 && i < 0xA0)
            return false;
        // No 0xFF char
        if(i == 0xFF)
            return false;
        // No surrogates and private use chars
        if(i >= 0xD800 && i <= 0xF8FF)
            return false;
    }
    return true;
}

// These functions are exported so that plugins can use this to detect a string, or replace with a plugin-developed string dection algorithm through hooking
extern "C" __declspec(dllexport) bool isasciistring(const unsigned char* data, int maxlen)
{
    int len = 0;
    for(const char* p = (const char*)data; *p; len++, p++)
    {
        if(len >= maxlen)
            break;
    }

    if(len < 2 || len + 1 >= maxlen)
        return false;

    String data2;
    WString wdata2;
    // Convert to and from Unicode
    wdata2 = StringUtils::LocalCpToUtf16((const char*)data);
    if(wdata2.size() < 2)
        return false;
    data2 = StringUtils::Utf16ToLocalCp(wdata2);
    if(data2.size() > maxlen || data2.size() < 2)
        return false;
    // Is the data exactly representable in both ANSI and Unicode?
    if(memcmp(data2.c_str(), data, data2.size()) != 0)
        return false;
    // Filter out bad chars
    if(!isunicodestring(wdata2))
        return false;
    return true;
}

extern "C" __declspec(dllexport) bool isunicodestring(const unsigned char* data, int maxlen)
{
    int len = 0;
    for(const wchar_t* p = (const wchar_t*)data; *p; len += sizeof(wchar_t), p++)
    {
        if(len >= maxlen)
            break;
    }

    if(len < 2 * sizeof(wchar_t) || len + 1 >= maxlen)
        return false;

    String data2;
    WString wdata2;
    // Convert to and from ANSI
    data2 = StringUtils::Utf16ToLocalCp((const wchar_t*)data);
    if(data2.size() < 2)
        return false;
    wdata2 = StringUtils::LocalCpToUtf16(data2);
    if(wdata2.size() / sizeof(wchar_t) > maxlen || wdata2.size() < 2)
        return false;
    // Is the data exactly representable in both ANSI and Unicode?
    if(memcmp(wdata2.c_str(), data, wdata2.size() * sizeof(wchar_t)) != 0)
        return false;
    // Filter out bad chars
    if(!isunicodestring(wdata2))
        return false;
    return true;
}

bool disasmispossiblestring(duint addr, STRING_TYPE* type)
{
    unsigned char data[60];
    memset(data, 0, sizeof(data));
    duint bytesRead = 0;
    if(!MemReadUnsafe(addr, data, sizeof(data) - 3, &bytesRead) && bytesRead < 2)
        return false;
    if(isasciistring(data, sizeof(data)))
    {
        if(type)
            *type = str_ascii;
        return true;
    }
    if(isunicodestring(data, sizeof(data) / 2))
    {
        if(type)
            *type = str_unicode;
        return true;
    }
    if(type)
        *type = str_none;
    return false;
}

bool disasmgetstringat(duint addr, STRING_TYPE* type, char* ascii, char* unicode, int maxlen)
{
    if(type)
        *type = str_none;
    if(!MemIsValidReadPtrUnsafe(addr, true) || !disasmispossiblestring(addr))
        return false;
    Memory<unsigned char*> data((maxlen + 1) * 2, "disasmgetstringat:data");
    MemReadUnsafe(addr, data(), (maxlen + 1) * 2); //TODO: use safe version?

    // Save a few pointer casts
    auto asciiData = (char*)data();

    // First check if this was an ASCII only string
    if(isasciistring(data(), maxlen))
    {
        if(type)
            *type = str_ascii;

        // Convert ANSI string to UTF-8
        std::string asciiData2 = StringUtils::LocalCpToUtf8((const char*)data());
        memcpy(asciiData, asciiData2.c_str(), min((size_t(maxlen) + 1) * 2, asciiData2.size() + 1));

        // Escape the string
        String escaped = StringUtils::Escape(asciiData);

        // Copy data back to outgoing parameter
        strncpy_s(ascii, min(int(escaped.length()) + 1, maxlen), escaped.c_str(), _TRUNCATE);
        return true;
    }

    if(isunicodestring(data(), maxlen))
    {
        if(type)
            *type = str_unicode;

        // Convert UTF-16 string to UTF-8
        std::string asciiData2 = StringUtils::Utf16ToUtf8((const wchar_t*)data());
        memcpy(asciiData, asciiData2.c_str(), min((size_t(maxlen) + 1) * 2, asciiData2.size() + 1));

        // Escape the string
        String escaped = StringUtils::Escape(asciiData);

        // Copy data back to outgoing parameter
        strncpy_s(unicode, min(int(escaped.length()) + 1, maxlen), escaped.c_str(), _TRUNCATE);
        return true;
    }

    return false;
}

bool disasmgetstringatwrapper(duint addr, char* dest, bool cache)
{
    if(!MemIsValidReadPtrUnsafe(addr, cache))
        return false;

    auto readValidPtr = [cache](duint addr) -> duint
    {
        duint addrPtr;
        if(MemReadUnsafe(addr, &addrPtr, sizeof(addrPtr)) && MemIsValidReadPtrUnsafe(addrPtr, cache))
            return addrPtr;
        return 0;
    };

    *dest = '\0';
    char string[MAX_STRING_SIZE];
    duint addrPtr = readValidPtr(addr);
    STRING_TYPE strtype;
    auto possibleUnicode = disasmispossiblestring(addr, &strtype) && strtype == str_unicode;
    if(addrPtr && !possibleUnicode)
    {
        if(disasmgetstringat(addrPtr, &strtype, string, string, MAX_STRING_SIZE - 5))
        {
            if(int(strlen(string)) <= (strtype == str_ascii ? 3 : 2) && readValidPtr(addrPtr))
                return false;
            if(strtype == str_ascii)
                sprintf_s(dest, MAX_STRING_SIZE, "&\"%s\"", string);
            else //unicode
                sprintf_s(dest, MAX_STRING_SIZE, "&L\"%s\"", string);
            return true;
        }
    }
    if(disasmgetstringat(addr, &strtype, string, string, MAX_STRING_SIZE - 4))
    {
        if(strtype == str_ascii)
            sprintf_s(dest, MAX_STRING_SIZE, "\"%s\"", string);
        else //unicode
            sprintf_s(dest, MAX_STRING_SIZE, "L\"%s\"", string);
        return true;
    }
    return false;
}

int disasmgetsize(duint addr, unsigned char* data)
{
    Zydis cp;
    if(!cp.Disassemble(addr, data, MAX_DISASM_BUFFER))
        return 1;
    return int(EncodeMapGetSize(addr, cp.Size()));
}

int disasmgetsize(duint addr)
{
    char data[MAX_DISASM_BUFFER];
    if(!MemRead(addr, data, sizeof(data)))
        return 1;
    return disasmgetsize(addr, (unsigned char*)data);
}
