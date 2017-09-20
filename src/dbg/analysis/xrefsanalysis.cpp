#include "xrefsanalysis.h"
#include "xrefs.h"
#include "console.h"

void XrefsAnalysis::Analyse()
{
    dputs("Starting xref analysis...");
    auto ticks = GetTickCount();

    for(auto addr = mBase; addr < mBase + mSize;)
    {
        if(!mCp.Disassemble(addr, translateAddr(addr)))
        {
            addr++;
            continue;
        }
        addr += mCp.Size();

        XREF xref;
        xref.addr = 0;
        xref.from = mCp.Address();
        for(auto i = 0; i < mCp.OpCount(); i++)
        {
            duint dest = mCp.ResolveOpValue(i, [](ZydisRegister)->size_t
            {
                return 0;
            });
            if(inRange(dest))
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
