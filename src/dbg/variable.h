#ifndef _VARIABLE_H
#define _VARIABLE_H

#include "_global.h"

//enums
enum VAR_TYPE
{
    VAR_SYSTEM = 1,
    VAR_USER = 2,
    VAR_READONLY = 3,
    VAR_HIDDEN = 4
};

enum VAR_VALUE_TYPE
{
    VAR_UINT,
    VAR_STRING,
};

//structures
struct VAR_VALUE
{
    union
    {
        duint value = 0;
        std::vector<unsigned char>* data;
    } u;
    VAR_VALUE_TYPE type = VAR_UINT;
    int size = 0;
};

struct VAR
{
    String name;
    String alias;
    VAR_TYPE type = VAR_SYSTEM;
    VAR_VALUE value;
};

struct CaseInsensitiveCompare
{
    bool operator()(const String & str1, const String & str2) const
    {
        return _stricmp(str1.c_str(), str2.c_str()) < 0;
    }
};

//functions
void varsetvalue(VAR* Var, VAR_VALUE* Value);
bool varset(const char* Name, VAR_VALUE* Value, bool ReadOnly);
void varinit();
void varfree();
bool varnew(const char* Name, duint Value, VAR_TYPE Type);
bool varget(const char* Name, VAR_VALUE* Value, int* Size, VAR_TYPE* Type);
bool varget(const char* Name, duint* Value, int* Size, VAR_TYPE* Type);
bool varget(const char* Name, char* String, int* Size, VAR_TYPE* Type);
bool varset(const char* Name, duint Value, bool ReadOnly);
bool varset(const char* Name, const char* Value, bool ReadOnly);
bool vardel(const char* Name, bool DelSystem);
bool vargettype(const char* Name, VAR_TYPE* Type = nullptr, VAR_VALUE_TYPE* ValueType = nullptr);
bool varenum(VAR* List, size_t* Size);

#endif // _VARIABLE_H
