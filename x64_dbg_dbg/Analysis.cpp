#include "Analysis.h"
#include "console.h"
#include "module.h"

void Derp()
{
    dputs("Starting analysis...");
    DWORD ticks = GetTickCount();

    uint _base = 0;
    uint modBase = ModBaseFromAddr(_base);
    uint modSize = ModSizeFromAddr(_base);

    LinearPass* pass = new LinearPass(modBase, modBase + modSize);
    pass->Analyse();
    /*

    PopulateReferences();
    dprintf("%u called functions populated\n", _functions.size());
    AnalyseFunctions();
    */
    dprintf("Analysis finished in %ums!\n", GetTickCount() - ticks);
}