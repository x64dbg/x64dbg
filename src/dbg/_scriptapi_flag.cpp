#include "_scriptapi_flag.h"
#include "value.h"

static const char* flagTable[] =
{
    "_ZF",
    "_OF",
    "_CF",
    "_PF",
    "_SF",
    "_TF",
    "_AF",
    "_DF",
    "_IF"
};

SCRIPT_EXPORT bool Script::Flag::Get(FlagEnum flag)
{
    duint value;
    return valfromstring(flagTable[flag], &value) ? !!value : false;
}

SCRIPT_EXPORT bool Script::Flag::Set(FlagEnum flag, bool value)
{
    return setflag(flagTable[flag], value);
}

SCRIPT_EXPORT bool Script::Flag::GetZF()
{
    return Get(ZF);
}

SCRIPT_EXPORT bool Script::Flag::SetZF(bool value)
{
    return Set(ZF, value);
}

SCRIPT_EXPORT bool Script::Flag::GetOF()
{
    return Get(OF);
}

SCRIPT_EXPORT bool Script::Flag::SetOF(bool value)
{
    return Set(OF, value);
}

SCRIPT_EXPORT bool Script::Flag::GetCF()
{
    return Get(CF);
}

SCRIPT_EXPORT bool Script::Flag::SetCF(bool value)
{
    return Set(CF, value);
}

SCRIPT_EXPORT bool Script::Flag::GetPF()
{
    return Get(PF);
}

SCRIPT_EXPORT bool Script::Flag::SetPF(bool value)
{
    return Set(PF, value);
}

SCRIPT_EXPORT bool Script::Flag::GetSF()
{
    return Get(SF);
}

SCRIPT_EXPORT bool Script::Flag::SetSF(bool value)
{
    return Set(SF, value);
}

SCRIPT_EXPORT bool Script::Flag::GetTF()
{
    return Get(TF);
}

SCRIPT_EXPORT bool Script::Flag::SetTF(bool value)
{
    return Set(TF, value);
}

SCRIPT_EXPORT bool Script::Flag::GetAF()
{
    return Get(AF);
}

SCRIPT_EXPORT bool Script::Flag::SetAF(bool value)
{
    return Set(AF, value);
}

SCRIPT_EXPORT bool Script::Flag::GetDF()
{
    return Get(DF);
}

SCRIPT_EXPORT bool Script::Flag::SetDF(bool value)
{
    return Set(DF, value);
}

SCRIPT_EXPORT bool Script::Flag::GetIF()
{
    return Get(IF);
}

SCRIPT_EXPORT bool Script::Flag::SetIF(bool value)
{
    return Set(IF, value);
}