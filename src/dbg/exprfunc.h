#pragma once

#include "_global.h"

namespace Exprfunc
{
    duint srcline(duint addr);
    duint srcdisp(duint addr);

    duint modparty(duint addr);

    duint disasmsel();
    duint dumpsel();
    duint stacksel();

    duint peb();
    duint teb();
    duint tid();

    duint bswap(duint value);
    duint ternary(duint condition, duint value1, duint value2);

    duint memvalid(duint addr);
    duint membase(duint addr);
    duint memsize(duint addr);
    duint memiscode(duint addr);
    duint memdecodepointer(duint ptr);

    duint dislen(duint addr);
    duint disiscond(duint addr);
    duint disisbranch(duint addr);
    duint disisret(duint addr);
    duint disismem(duint addr);
    duint disbranchdest(duint addr);
    duint disbranchexec(duint addr);
    duint disimm(duint addr);
    duint disbrtrue(duint addr);
    duint disbrfalse(duint addr);

    duint trenabled(duint addr);
    duint trhitcount(duint addr);
    duint gettickcount();
    duint sleep(duint ms);

    duint readbyte(duint addr);
    duint readword(duint addr);
    duint readdword(duint addr);
    duint readqword(duint addr);
    duint readptr(duint addr);
}