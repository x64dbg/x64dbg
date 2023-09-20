#include "xrefsanalysis.h"
#include "xrefs.h"
#include "console.h"
#include <module.h>

XrefsAnalysis::XrefsAnalysis(duint base, duint size)
    : Analysis(base, size)
{
    modbase = ModBaseFromAddr(base);
    modsize = ModSizeFromAddr(modbase);
}

void XrefsAnalysis::Analyse()
{
    dputs("Starting xref analysis...");
    auto ticks = GetTickCount();

    for(auto addr = mBase; addr < mBase + mSize;)
    {
        if(!mZydis.Disassemble(addr, translateAddr(addr)))
        {
            addr++;
            continue;
        }
        addr += mZydis.Size();

        XREF xref;
        xref.addr = 0;
        xref.from = (duint)mZydis.Address();
        for(auto i = 0; i < mZydis.OpCount(); i++)
        {
            auto dest = (duint)mZydis.ResolveOpValue(i, [](ZydisRegister) -> uint64_t
            {
                return 0;
            });
            if(inModRange(dest))
            {
                xref.addr = dest;
                break;
            }
        }
        if(xref.addr)
            mXrefs.push_back(xref);
    }

    dprintf("%u xrefs found in %ums!\n", DWORD(mXrefs.size()), GetTickCount() - ticks);
}

void XrefsAnalysis::SetMarkers()
{
    XrefDelRange(mBase, mBase + mSize - 1);
    for(const auto & xref : mXrefs)
        XrefAdd(xref.addr, xref.from);
}
