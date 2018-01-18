#include "analysis_nukem.h"
#include "BasicBlock.h"
#include "LinearPass.h"
#include "FunctionPass.h"
#include "console.h"

void Analyse_nukem(duint base, duint size)
{
    dputs("Starting analysis (Nukem)...");
    DWORD ticks = GetTickCount();

    duint end = base + size;

    BBlockArray blocks;

    LinearPass pass1(base, end, blocks);
    pass1.Analyse();

    FunctionPass pass2(base, end, blocks);
    pass2.Analyse();

    dprintf("Analysis finished in %ums!\n", GetTickCount() - ticks);
}