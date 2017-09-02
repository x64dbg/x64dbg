#pragma once

struct CookieQuery
{
    duint addr = 0;
    duint ret = 0;
    duint cookieptr = 0;
    ULONG cookie = 0;
    bool removeAddrBp = false;
    bool removeRetBp = false;

    void HandleNtdllLoad()
    {
        *this = CookieQuery();
        if(valfromstring("ntdll.dll:NtQueryInformationProcess", &addr))
        {
            if(!BpGet(addr, BPNORMAL, nullptr, nullptr))
            {
                if(SetBPX(addr, UE_BREAKPOINT, (void*)cbUserBreakpoint))
                    removeAddrBp = true;
                else
                    addr = 0;
            }
        }
    }

    bool HandleBreakpoint(duint cip)
    {
        if(addr && addr == cip && Exprfunc::argget(1) == ProcessCookie)
        {
            if(valfromstring("[csp]", &ret))
            {
                cookieptr = Exprfunc::argget(2);
                if(!BpGet(ret, BPNORMAL, nullptr, nullptr))
                {
                    if(SetBPX(ret, UE_BREAKPOINT, (void*)cbUserBreakpoint))
                        removeRetBp = true;
                    else
                        ret = 0;
                }
                else
                    removeRetBp = false;
            }
            if(removeAddrBp)
            {
                DeleteBPX(addr);
                BpDelete(addr, BPNORMAL);
                return true;
            }
        }
        else if(ret && ret == cip)
        {
            if(!MemRead(cookieptr, &cookie, sizeof(cookie)))
                cookie = 0, cookieptr = 0;
            if(removeRetBp)
            {
                DeleteBPX(ret);
                BpDelete(ret, BPNORMAL);
                return true;
            }
        }
        return false;
    }
};