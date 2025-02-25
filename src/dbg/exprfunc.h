#pragma once

#include "_global.h"
#include "expressionfunctions.h"

namespace Exprfunc
{
    duint srcline(duint addr);
    duint srcdisp(duint addr);

    duint modparty(duint addr);
    duint modsystem(duint addr);
    duint moduser(duint addr);
    duint modrva(duint addr);
    duint modheaderva(duint addr);
    duint modisexport(duint addr);
    bool modbasefromname(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata);

    duint disasmsel();
    duint dumpsel();
    duint stacksel();

    duint peb();
    duint teb();
    duint tid();
    duint kusd();

    duint bswap(duint value);
    bool ternary(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata);

    duint memvalid(duint addr);
    duint membase(duint addr);
    duint memsize(duint addr);
    duint memiscode(duint addr);
    duint memisstring(duint addr);
    duint memdecodepointer(duint ptr);
    bool memmatch(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata);

    duint dislen(duint addr);
    duint disiscond(duint addr);
    duint disisbranch(duint addr);
    duint disisret(duint addr);
    duint disiscall(duint addr);
    duint disismem(duint addr);
    duint disisnop(duint addr);
    duint disisunusual(duint addr);
    duint disbranchdest(duint addr);
    duint disbranchexec(duint addr);
    duint disimm(duint addr);
    duint disbrtrue(duint addr);
    duint disbrfalse(duint addr);
    duint disnext(duint addr);
    duint disprev(duint addr);
    duint disiscallsystem(duint addr);
    bool dismnemonic(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata);
    bool distext(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata);
    bool dismatch(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata);

    duint trenabled(duint addr);
    duint trhitcount(duint addr);
    duint trisrecording();
    duint gettickcount();
    duint rdtsc();

    duint readbyte(duint addr);
    duint readword(duint addr);
    duint readdword(duint addr);
    duint readqword(duint addr);
    duint readptr(duint addr);

    duint funcstart(duint addr);
    duint funcend(duint addr);

    duint refcount();
    duint refaddr(duint row);
    duint refsearchcount();
    duint refsearchaddr(duint row);

    duint argget(duint index);
    duint argset(duint index, duint value);

    duint bpgoto(duint cip);

    duint exfirstchance();
    duint exaddr();
    duint excode();
    duint exflags();
    duint exinfocount();
    duint exinfo(duint index);

    duint isdebuggerfocused();
    duint isdebuggeefocused();

    bool streq(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata);
    bool strieq(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata);
    bool strstr(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata);
    bool stristr(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata);
    bool strlen(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata);
    bool ansi(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata);
    bool ansi_strict(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata);
    bool utf8(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata);
    bool utf8_strict(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata);
    bool utf16(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata);
    bool utf16_strict(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata);

    bool syscall_name(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata);
    bool syscall_id(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata);
}