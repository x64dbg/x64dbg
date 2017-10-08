/**
 @file assemble.cpp

 @brief Implements the assemble class.
 */

#include "assemble.h"
#include "memory.h"
#include "XEDParse/XEDParse.h"
#include "value.h"
#include "disasm_fast.h"
#include "disasm_helper.h"
#include "keystone/keystone.h"
#include "datainst_helper.h"
#include "debugger.h"

AssemblerEngine assemblerEngine = AssemblerEngine::XEDParse;

namespace Keystone
{
    static char* stristr(const char* haystack, const char* needle)
    {
        // Case insensitive strstr
        // https://stackoverflow.com/questions/27303062/strstr-function-like-that-ignores-upper-or-lower-case
        do
        {
            const char* h = haystack;
            const char* n = needle;
            while(tolower((unsigned char)*h) == tolower((unsigned char)*n) && *n)
            {
                h++;
                n++;
            }

            if(*n == 0)
                return (char*)haystack;

        }
        while(*haystack++);

        // Not found
        return nullptr;
    }

    static bool StrDel(char* Source, const char* Needle)
    {
        // Find the location in the string first
        char* loc = stristr(Source, Needle);

        if(!loc)
            return false;

        // "Delete" the word by shifting it over
        auto needleLen = strlen(Needle);

        memmove(loc, loc + needleLen, strlen(loc) - needleLen + 1);

        return true;
    }

    static XEDPARSE_STATUS XEDParseAssemble(XEDPARSE* XEDParse)
    {
        if(!XEDParse)
            return XEDPARSE_ERROR;

        auto sep = strstr(XEDParse->instr, ";");
        if(!sep)
            sep = strstr(XEDParse->instr, "\n");
        if(sep)
            *sep = '\0';
        bool short_command = StrDel(XEDParse->instr, "short ");

        ks_engine* ks;
        ks_err err = ks_open(KS_ARCH_X86, XEDParse->x64 ? KS_MODE_64 : KS_MODE_32, &ks);
        if(err != KS_ERR_OK)
        {
            strcpy_s(XEDParse->error, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Failed on ks_open()...")));
            return XEDPARSE_ERROR;
        }
        //ks_option(ks, KS_OPT_SYNTAX, KS_OPT_SYNTAX_INTEL);
        auto result = XEDPARSE_OK;
        unsigned char* encode;
        size_t size;
        size_t count;
        if(ks_asm(ks, XEDParse->instr, XEDParse->cip, &encode, &size, &count) != KS_ERR_OK)
        {
            sprintf_s(XEDParse->error, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "ks_asm() failed: count = %lu, error = %u")), count, ks_errno(ks));
            result = XEDPARSE_ERROR;
        }
        else if(short_command && size > 2)
        {
            sprintf_s(XEDParse->error, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "ks_asm() failed: destination is out of range (size = %lu)")), size);
            result = XEDPARSE_ERROR;
        }
        else
        {
            XEDParse->dest_size = (unsigned int)min(size, XEDPARSE_MAXASMSIZE);
            for(size_t i = 0; i < XEDParse->dest_size; i++)
                XEDParse->dest[i] = encode[i];
        }
        ks_free(encode);
        ks_close(ks);
        return result;
    }
}

namespace asmjit
{
    static XEDPARSE_STATUS XEDParseAssemble(XEDPARSE* XEDParse)
    {
        static auto asmjitAssemble = (XEDPARSE_STATUS(*)(XEDPARSE*))GetProcAddress(LoadLibraryW(L"asmjit.dll"), "XEDParseAssemble");
        if(asmjitAssemble)
            return asmjitAssemble(XEDParse);
        strcpy_s(XEDParse->error, "asmjit not found!");
        return XEDPARSE_ERROR;
    }
}

static bool cbUnknown(const char* text, ULONGLONG* value)
{
    if(!text || !value)
        return false;
    duint val;
    if(!valfromstring(text, &val))
        return false;
    *value = val;
    return true;
}

bool assemble(duint addr, unsigned char* dest, int destsize, int* size, const char* instruction, char* error)
{
    if(isdatainstruction(instruction))
    {
        return tryassembledata(addr, dest, destsize, size, instruction, error);
    }
    if(strlen(instruction) >= XEDPARSE_MAXBUFSIZE)
        return false;
    XEDPARSE parse;
    memset(&parse, 0, sizeof(parse));
#ifdef _WIN64
    parse.x64 = true;
#else //x86
    parse.x64 = false;
#endif
    parse.cbUnknown = cbUnknown;
    parse.cip = addr;
    strcpy_s(parse.instr, instruction);
    auto DoAssemble = XEDParseAssemble;
    if(assemblerEngine == AssemblerEngine::Keystone)
        DoAssemble = Keystone::XEDParseAssemble;
    else if(assemblerEngine == AssemblerEngine::asmjit)
        DoAssemble = asmjit::XEDParseAssemble;
    if(DoAssemble(&parse) == XEDPARSE_ERROR)
    {
        if(error)
            strcpy_s(error, MAX_ERROR_SIZE, parse.error);
        return false;
    }

    if(dest)
        memcpy(dest, parse.dest, parse.dest_size);
    if(size)
        *size = parse.dest_size;

    return true;
}

bool assemble(duint addr, unsigned char* dest, int* size, const char* instruction, char* error)
{
    return assemble(addr, dest, 16, size, instruction, error);
}

static bool isInstructionPointingToExMemory(duint addr, const unsigned char* dest)
{
    BASIC_INSTRUCTION_INFO basicinfo;
    // Check if the instruction changes CIP and if it does not pretent it does point to valid executable memory.
    if(!disasmfast(dest, addr, &basicinfo) || !basicinfo.branch || (basicinfo.type & TYPE_ADDR) == 0)
        return true;

    // An instruction pointing to invalid memory does not point to executable memory.
    if(!MemIsValidReadPtr(basicinfo.addr))
        return false;

    // Check if memory region is marked as executable
    if(MemIsCodePage(basicinfo.addr, false))
        return true;

    // Check if DEP is disabled
    return !dbgisdepenabled();
}

bool assembleat(duint addr, const char* instruction, int* size, char* error, bool fillnop)
{
    int destSize = 0;
    Memory<unsigned char*> dest(16 * sizeof(unsigned char), "AssembleBuffer");
    unsigned char* newbuffer = nullptr;
    if(!assemble(addr, dest(), 16, &destSize, instruction, error))
    {
        if(destSize > 16)
        {
            dest.realloc(destSize);
            if(!assemble(addr, dest(), destSize, &destSize, instruction, error))
                return false;
        }
        else
            return false;
    }

    //calculate the number of NOPs to insert
    int origLen = disasmgetsize(addr);
    while(origLen < destSize)
        origLen += disasmgetsize(addr + origLen);
    int nopsize = origLen - destSize;
    unsigned char nops[16];
    memset(nops, 0x90, sizeof(nops));

    if(size)
        *size = destSize;

    // Check if the instruction doesn't set IP to non-executable memory
    if(!isInstructionPointingToExMemory(addr, dest()))
    {
        String Title;
        String Text;
        Title = GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Non-executable memory region"));
        Text = GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Assembled branch does not point to an executable memory region!"));
        GuiDisplayWarning(Title.c_str(), Text.c_str());
    }

    bool ret = MemPatch(addr, dest(), destSize);

    if(ret)
    {
        if(fillnop && nopsize)
        {
            if(size)
                *size += nopsize;

            // Ignored if the memory patch for NOPs fail (although it should not)
            MemPatch(addr + destSize, nops, nopsize);
        }

        // Update GUI if any patching succeeded
        GuiUpdatePatches();
    }
    else
    {
        // Tell the user writing is blocked
        strcpy_s(error, MAX_ERROR_SIZE, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Error while writing process memory")));
    }

    return ret;
}