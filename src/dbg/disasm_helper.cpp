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
#include <capstone_wrapper.h>
#include "datainst_helper.h"

duint disasmback(unsigned char* data, duint base, duint size, duint ip, int n)
{
    int i;
    duint abuf[131], addr, back, cmdsize;
    memset(abuf, 0, sizeof(abuf));
    unsigned char* pdata;

    // Reset Disasm Structure
    Capstone cp;

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
    Capstone cp;

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

static void HandleCapstoneOperand(Capstone & cp, int opindex, DISASM_ARG* arg, bool getregs)
{
    auto value = cp.ResolveOpValue(opindex, [&cp, getregs](x86_reg reg)
    {
        auto regName = getregs ? cp.RegName(reg) : nullptr;
        return regName ? getregister(nullptr, regName) : 0; //TODO: temporary needs enums + caching
    });
    const auto & op = cp[opindex];
    arg->segment = SEG_DEFAULT;
    strcpy_s(arg->mnemonic, cp.OperandText(opindex).c_str());
    switch(op.type)
    {
    case X86_OP_REG:
    {
        arg->type = arg_normal;
        arg->value = value;
    }
    break;

    case X86_OP_IMM:
    {
        arg->type = arg_normal;
        arg->constant = arg->value = value;
    }
    break;

    case X86_OP_MEM:
    {
        arg->type = arg_memory;
        const x86_op_mem & mem = op.mem;
        if(mem.base == X86_REG_RIP) //rip-relative
            arg->constant = cp.Address() + duint(mem.disp) + cp.Size();
        else
            arg->constant = duint(mem.disp);
#ifdef _WIN64
        if(mem.segment == X86_REG_GS)
        {
            arg->segment = SEG_GS;
#else //x86
        if(mem.segment == X86_REG_FS)
        {
            arg->segment = SEG_FS;
#endif
            value += ThreadGetLocalBase(ThreadGetId(hActiveThread));
        }
        arg->value = value;
        if(DbgMemIsValidReadPtr(value))
        {
            switch(op.size)
            {
            case 1:
                MemRead(value, (unsigned char*)&arg->memvalue, 1);
                break;
            case 2:
                MemRead(value, (unsigned char*)&arg->memvalue, 2);
                break;
            case 4:
                MemRead(value, (unsigned char*)&arg->memvalue, 4);
                break;
#ifdef _WIN64
            case 8:
                MemRead(value, (unsigned char*)&arg->memvalue, 8);
                break;
#endif //_WIN64
            }
        }
    }
    break;

    default:
        break;
    }
}

void disasmget(Capstone & cp, unsigned char* buffer, duint addr, DISASM_INSTR* instr, bool getregs)
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
    const cs_insn* cpInstr = cp.GetInstr();
    sprintf_s(instr->instruction, "%s %s", cpInstr->mnemonic, cpInstr->op_str);
    instr->instr_size = cpInstr->size;
    if(cp.InGroup(CS_GRP_JUMP) || cp.IsLoop() || cp.InGroup(CS_GRP_RET) || cp.InGroup(CS_GRP_CALL))
        instr->type = instr_branch;
    else if(strstr(cpInstr->op_str, "sp") || strstr(cpInstr->op_str, "bp"))
        instr->type = instr_stack;
    else
        instr->type = instr_normal;
    instr->argcount = cp.x86().op_count <= 3 ? cp.x86().op_count : 3;
    for(int i = 0; i < instr->argcount; i++)
        HandleCapstoneOperand(cp, i, &instr->arg[i], getregs);
}

void disasmget(Capstone & cp, duint addr, DISASM_INSTR* instr, bool getregs)
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
    Capstone cp;
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

extern "C" __declspec(dllexport) bool isasciistring(const unsigned char* data, int maxlen)
{
    int len = 0;
    for(char* p = (char*)data; *p; len++, p++)
    {
        if(len >= maxlen)
            break;
    }

    if(len < 2 || len + 1 >= maxlen)
        return false;
    for(int i = 0; i < len; i++)
        if(!isprint(data[i]) && !isspace(data[i]))
            return false;
    return true;
}

extern "C" __declspec(dllexport) bool isunicodestring(const unsigned char* data, int maxlen)
{
    int len = 0;
    for(wchar_t* p = (wchar_t*)data; *p; len++, p++)
    {
        if(len >= maxlen)
            break;
    }

    if(len < 2 || len + 1 >= maxlen)
        return false;

    for(int i = 0; i < len * 2; i += 2)
    {
        if(data[i + 1]) //Extended ASCII only
            return false;
        if(!isprint(data[i]) && !isspace(data[i]))
            return false;
    }
    return true;
}

bool disasmispossiblestring(duint addr, STRING_TYPE* type)
{
    unsigned char data[11];
    memset(data, 0, sizeof(data));
    if(!MemReadUnsafe(addr, data, sizeof(data) - 3))
        return false;
    duint test = 0;
    memcpy(&test, data, sizeof(duint));
    if(isasciistring(data, sizeof(data)))
    {
        if(type)
            *type = str_ascii;
        return true;
    }
    if(isunicodestring(data, _countof(data)))
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
    if(!MemReadUnsafe(addr, data(), (maxlen + 1) * 2)) //TODO: use safe version?
        return false;

    // Save a few pointer casts
    auto asciiData = (char*)data();
    auto unicodeData = (wchar_t*)data();

    // First check if this was an ASCII only string
    if(isasciistring(data(), maxlen))
    {
        if(type)
            *type = str_ascii;

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

        // Determine string length only once, limited to output buffer size
        int unicodeLength = min(int(wcslen(unicodeData)), maxlen);

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
    Capstone cp;
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
