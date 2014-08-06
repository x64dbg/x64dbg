#include "variable.h"

static VariableMap variables;
static VAR* vars;

static void varsetvalue(VAR* var, VAR_VALUE* value)
{
    switch(var->value.type)
    {
    case VAR_STRING:
        var->value.u.data->clear();
        delete var->value.u.data;
        break;
    default:
        break;
    }
    memcpy(&var->value, value, sizeof(VAR_VALUE));
}

static bool varset(const char* name, VAR_VALUE* value, bool setreadonly)
{
    std::string name_;
    if(*name != '$')
        name_ = "$";
    name_ += name;
    VariableMap::iterator found = variables.find(name_);
    if(found == variables.end()) //not found
        return false;
    if(!setreadonly && (found->second.type == VAR_READONLY || found->second.type == VAR_HIDDEN))
        return false;
    varsetvalue(&found->second, value);
    return true;
}

void varinit()
{
    variables.clear();
    //General variables
    varnew("$res\1$result", 0, VAR_SYSTEM);
    varnew("$res1\1$result1", 0, VAR_SYSTEM);
    varnew("$res2\1$result2", 0, VAR_SYSTEM);
    varnew("$res3\1$result3", 0, VAR_SYSTEM);
    varnew("$res4\1$result4", 0, VAR_SYSTEM);
    //InitDebug variables
    varnew("$hp\1$hProcess", 0, VAR_READONLY);
    varnew("$pid", 0, VAR_READONLY);
    //hidden variables
    varnew("$ans\1$an", 0, VAR_HIDDEN);
    //read-only variables
    varnew("$lastalloc", 0, VAR_READONLY);
    varnew("$_EZ_FLAG", 0, VAR_READONLY); //equal/zero flag for internal use (1=equal, 0=unequal)
    varnew("$_BS_FLAG", 0, VAR_READONLY); //bigger/smaller flag for internal use (1=bigger, 0=smaller)
}

void varfree()
{
    variables.clear();
}

VAR* vargetptr()
{
    return 0;
}

bool varnew(const char* name, uint value, VAR_TYPE type)
{
    if(!name)
        return false;
    std::string name_;
    if(*name != '$')
        name_ = "$";
    name_ += name;
    if(variables.find(name_) != variables.end()) //found
        return false;
    VAR var;
    var.name = name_;
    var.type = type;
    VAR_VALUE varvalue;
    varvalue.size = sizeof(uint);
    varvalue.type = VAR_UINT;
    varvalue.u.value = value;
    varsetvalue(&var, &varvalue);
    variables.insert(std::make_pair(name_, var));
    return true;
}

static bool varget(const char* name, VAR_VALUE* value, int* size, VAR_TYPE* type)
{
    std::string name_;
    if(*name != '$')
        name_ = "$";
    name_ += name;
    VariableMap::iterator found = variables.find(name_);
    if(found == variables.end()) //not found
        return false;
    *type = found->second.type;
    *size = found->second.value.size;
    *value = found->second.value;
    return true;
}

bool varget(const char* name, uint* value, int* size, VAR_TYPE* type)
{
    VAR_VALUE varvalue;
    int varsize;
    VAR_TYPE vartype;
    if(!varget(name, &varvalue, &varsize, &vartype) or varvalue.type != VAR_UINT)
        return false;
    if(size)
        *size = varsize;
    if(!value && size)
        return true; //variable was valid, just get the size
    if(type)
        *type = vartype;
    *value = varvalue.u.value;
    return true;
}

bool varget(const char* name, char* string, int* size, VAR_TYPE* type)
{
    VAR_VALUE varvalue;
    int varsize;
    VAR_TYPE vartype;
    if(!varget(name, &varvalue, &varsize, &vartype) or varvalue.type != VAR_STRING)
        return false;
    if(size)
        *size = varsize;
    if(!string && size)
        return true; //variable was valid, just get the size
    if(type)
        *type = vartype;
    memcpy(string, &varvalue.u.data->front(), varsize);
    return true;
}

bool varset(const char* name, uint value, bool setreadonly)
{
    VAR_VALUE varvalue;
    varvalue.size = sizeof(uint);
    varvalue.type = VAR_UINT;
    varvalue.u.value = value;
    varset(name, &varvalue, setreadonly);
    return true;
}

bool varset(const char* name, const char* string, bool setreadonly)
{
    VAR_VALUE varvalue;
    int size = (int)strlen(string);
    varvalue.size = size;
    varvalue.type = VAR_STRING;
    varvalue.u.data = new std::vector<unsigned char>;
    varvalue.u.data->resize(size);
    memcpy(&varvalue.u.data->front(), string, size);
    if(!varset(name, &varvalue, setreadonly))
    {
        varvalue.u.data->clear();
        delete varvalue.u.data;
        return false;
    }
    return true;
}

bool vardel(const char* name, bool delsystem)
{
    std::string name_;
    if(*name != '$')
        name_ = "$";
    name_ += name;
    VariableMap::iterator found = variables.find(name_);
    if(found == variables.end()) //not found
        return false;
    if(!delsystem && found->second.type != VAR_USER)
        return false;
    variables.erase(found);
    return true;
}

bool vargettype(const char* name, VAR_TYPE* type, VAR_VALUE_TYPE* valtype)
{
    std::string name_;
    if(*name != '$')
        name_ = "$";
    name_ += name;
    VariableMap::iterator found = variables.find(name_);
    if(found == variables.end()) //not found
        return false;
    if(valtype)
        *valtype = found->second.value.type;
    if(type)
        *type = found->second.type;
    return true;
}

bool varenum(VAR* entries, size_t* cbsize)
{
    if(!entries && !cbsize || !variables.size())
        return false;
    if(!entries && cbsize)
    {
        *cbsize = variables.size() * sizeof(VAR);
        return true;
    }
    int j = 0;
    for(VariableMap::iterator i = variables.begin(); i != variables.end(); ++i, j++)
        entries[j] = i->second;
    return true;
}