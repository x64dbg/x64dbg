#ifndef _SCRIPTAPI_FLAG_H
#define _SCRIPTAPI_FLAG_H

#include "_scriptapi.h"

namespace Script
{
    namespace Flag
    {
        enum FlagEnum
        {
            ZF,
            OF,
            CF,
            PF,
            SF,
            TF,
            AF,
            DF,
            IF
        };

        SCRIPT_EXPORT bool Get(FlagEnum flag);
        SCRIPT_EXPORT bool Set(FlagEnum flag, bool value);

        SCRIPT_EXPORT bool GetZF();
        SCRIPT_EXPORT bool SetZF(bool value);
        SCRIPT_EXPORT bool GetOF();
        SCRIPT_EXPORT bool SetOF(bool value);
        SCRIPT_EXPORT bool GetCF();
        SCRIPT_EXPORT bool SetCF(bool value);
        SCRIPT_EXPORT bool GetPF();
        SCRIPT_EXPORT bool SetPF(bool value);
        SCRIPT_EXPORT bool GetSF();
        SCRIPT_EXPORT bool SetSF(bool value);
        SCRIPT_EXPORT bool GetTF();
        SCRIPT_EXPORT bool SetTF(bool value);
        SCRIPT_EXPORT bool GetAF();
        SCRIPT_EXPORT bool SetAF(bool value);
        SCRIPT_EXPORT bool GetDF();
        SCRIPT_EXPORT bool SetDF(bool value);
        SCRIPT_EXPORT bool GetIF();
        SCRIPT_EXPORT bool SetIF(bool value);
    };
};

#endif //_SCRIPTAPI_FLAG_H