#ifndef _VARIABLE_H
#define _VARIABLE_H

#include "_global.h"

//TODO: VAR_VALUE struct usage

//enums
enum VAR_TYPE
{
    VAR_SYSTEM=1,
    VAR_USER=2,
    VAR_READONLY=3,
    VAR_HIDDEN=4
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
    char* name;
    VAR_TYPE type;
    VAR_VALUE value;
    VAR* next;
};

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
bool vargettype(const char* name, VAR_TYPE* type);

#endif // _VARIABLE_H
