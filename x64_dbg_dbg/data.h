#ifndef _DATA_H
#define _DATA_H

#include "_global.h"

enum DATA_TYPE //how to display the current struct entry?
{
    thex, //%X
    tint, //%d
    tuint, //%u
    ttext, //%c
};

struct STRUCT_INFO
{
    unsigned int size; //size of one entry (with type) (max 256)
    DATA_TYPE display_type; //display type
    unsigned int count; //number of entries with the same content (reserved[12])
    void* description; //reserved for later use (for example name of variable)
};

struct DATA
{
    uint page_start; //remote/local memory
    uint page_size; //size of memory
    uint ip; //real start of data (relative from page_start)
    int struct_size; //number of entries in a struct
    STRUCT_INFO* info; //actual info
};

#endif // _DATA_H
