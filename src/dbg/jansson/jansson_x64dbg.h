typedef json_t* JSON;

static JSON_INLINE
json_t* json_hex(unsigned json_int_t value)
{
    char hexvalue[20];
#ifdef _WIN64
    sprintf(hexvalue, "0x%llX", value);
#else //x64
    sprintf(hexvalue, "0x%X", value);
#endif //_WIN64
    return json_string(hexvalue);
}

static JSON_INLINE
unsigned json_int_t json_hex_value(const json_t* hex)
{
    unsigned json_int_t ret;
    const char* hexvalue;
    hexvalue = json_string_value(hex);
    if(!hexvalue)
        return 0;
#ifdef _WIN64
    sscanf(hexvalue, "0x%llX", &ret);
#else //x64
    sscanf(hexvalue, "0x%X", &ret);
#endif //_WIN64
    return ret;
}