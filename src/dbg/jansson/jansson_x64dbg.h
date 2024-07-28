#pragma once

#include "jansson.h"

typedef json_t* JSON;

static JSON_INLINE
json_t* json_hex(unsigned json_int_t value)
{
    char hexvalue[20];
    sprintf_s(hexvalue, "0x%llX", value);
    return json_string(hexvalue);
}

static JSON_INLINE
unsigned json_int_t json_hex_value(const json_t* hex)
{
    unsigned json_int_t ret = 0;
    const char* hexvalue;
    hexvalue = json_string_value(hex);
    if(!hexvalue)
        return 0;
    sscanf_s(hexvalue, "0x%llX", &ret);
    return ret;
}

static JSON_INLINE
json_t* json_string(const std::string & str)
{
    return json_stringn(str.c_str(), str.length());
}
