#include "Analysis.h"
#include "console.h"
#include "module.h"

void Derp(uint _base)
{
    dputs("Starting analysis...");
    DWORD ticks = GetTickCount();

    uint modBase = ModBaseFromAddr(_base);
    uint modSize = ModSizeFromAddr(_base);

    BBlockArray array;
    LinearPass* pass = new LinearPass(modBase, modBase + modSize, array);
    pass->Analyse();

    //Int3CoagulatorPass *pass2 = new Int3CoagulatorPass(modBase, modBase + modSize, array);
    //pass2->Analyse();
    /*

    PopulateReferences();
    dprintf("%u called functions populated\n", _functions.size());
    AnalyseFunctions();
    */
    dprintf("Analysis finished in %ums!\n", GetTickCount() - ticks);
}