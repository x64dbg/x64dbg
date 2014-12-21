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
        uint value;
        std::vector<unsigned char>* data;
    } u;
    VAR_VALUE_TYPE type;
    int size;
};

struct VAR
{
    String name;
    String alias;
    VAR_TYPE type;
    VAR_VALUE value;
};

struct CaseInsensitiveCompare
{
    bool operator()(const String & str1, const String & str2) const
    {
        return _stricmp(str1.c_str(), str2.c_str()) < 0;
    }
};

typedef std::map<String, VAR, CaseInsensitiveCompare> VariableMap;

//functions
void varinit();
void varfree();
VAR* vargetptr();
bool varnew(const char* name, uint value, VAR_TYPE type);
bool varget(const char* name, uint* value, int* size, VAR_TYPE* type);
bool varget(const char* name, char* string, int* size, VAR_TYPE* type);
bool varset(const char* name, uint value, bool setreadonly);
bool varset(const char* name, const char* string, bool setreadonly);
bool vardel(const char* name, bool delsystem);
bool vargettype(const char* name, VAR_TYPE* type = 0, VAR_VALUE_TYPE* valtype = 0);
bool varenum(VAR* entries, size_t* cbsize);

#endif // _VARIABLE_H
