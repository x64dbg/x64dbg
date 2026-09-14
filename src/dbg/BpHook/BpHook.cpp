#include "BpHook.h"
#include "_plugins.h"
#include "plugin_loader.h"

bool SetBPXHooked(ULONG_PTR bpxAddress, DWORD bpxType, TITANCBSOFTBP bpxCallBack)
{
    PLUG_CB_BEFORE_SETBPX before;
    before.addr = (duint)bpxAddress;
    before.titantype = bpxType;
    before.callback = (void*)bpxCallBack;
    before.cancel = false;
    plugincbcall(CB_BEFORE_SETBPX, &before);

    // A plugin refused the breakpoint. Report failure so the caller runs the
    // rollback it already has for a failed SetBPX instead of leaving a stale
    // entry in the breakpoint database.
    if(before.cancel)
        return false;

    auto success = SetBPX(bpxAddress, bpxType, bpxCallBack);

    PLUG_CB_AFTER_SETBPX after;
    after.addr = (duint)bpxAddress;
    after.titantype = bpxType;
    after.callback = (void*)bpxCallBack;
    after.success = success;
    plugincbcall(CB_AFTER_SETBPX, &after);

    return success;
}
