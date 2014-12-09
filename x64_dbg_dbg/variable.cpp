#include "variable.h"
#include "threading.h"

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
    CriticalSectionLocker locker(LockVariables);
    String name_;
    if(*name != '$')
        name_ = "$";
    name_ += name;
    VariableMap::iterator found = variables.find(name_);
    if(found == variables.end()) //not found
        return false;
    if(found->second.alias.length())
        return varset(found->second.alias.c_str(), value, setreadonly);
    if(!setreadonly && (found->second.type == VAR_READONLY || found->second.type == VAR_HIDDEN))
        return false;
    varsetvalue(&found->second, value);
    return true;
}

void varinit()
{
    varfree();
    //General variables
    varnew("$result\1$res", 0, VAR_SYSTEM);
    varnew("$result1\1$res1", 0, VAR_SYSTEM);
    varnew("$result2\1$res2", 0, VAR_SYSTEM);
    varnew("$result3\1$res3", 0, VAR_SYSTEM);
    varnew("$result4\1$res4", 0, VAR_SYSTEM);
    //InitDebug variables
    varnew("$hProcess\1$hp", 0, VAR_READONLY);
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
    CriticalSectionLocker locker(LockVariables);
    variables.clear();
}

VAR* vargetptr()
{
    return 0;
}

bool varnew(const char* name, uint value, VAR_TYPE type)
{
    CriticalSectionLocker locker(LockVariables);
    if(!name)
        return false;
    std::vector<String> names = StringUtils::Split(name, '\1');
    String firstName;
    for(int i = 0; i < (int)names.size(); i++)
    {
        String name_;
        name = names.at(i).c_str();
        if(*name != '$')
            name_ = "$";
        name_ += name;
        if(!i)
            firstName = name;
        if(variables.find(name_) != variables.end()) //found
            return false;
        VAR var;
        var.name = name_;
        if(i)
            var.alias = firstName;
        var.type = type;
        var.value.size = sizeof(uint);
        var.value.type = VAR_UINT;
        var.value.u.value = value;
        variables.insert(std::make_pair(name_, var));
    }
    return true;
}

static bool varget(const char* name, VAR_VALUE* value, int* size, VAR_TYPE* type)
{
    CriticalSectionLocker locker(LockVariables);
    String name_;
    if(*name != '$')
        name_ = "$";
    name_ += name;
    VariableMap::iterator found = variables.find(name_);
    if(found == variables.end()) //not found
        return false;
    if(found->second.alias.length())
        return varget(found->second.alias.c_str(), value, size, type);
    if(type)
        *type = found->second.type;
    if(size)
        *size = found->second.value.size;
    if(value)
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
    if(value)
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
    if(string)
        memcpy(string, &varvalue.u.data->front(), varsize);
    return true;
}

bool varset(const char* name, uint value, bool setreadonly)
{
    VAR_VALUE varvalue;
    varvalue.size = sizeof(uint);
    varvalue.type = VAR_UINT;
    varvalue.u.value = value;
    return varset(name, &varvalue, setreadonly);
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
    CriticalSectionLocker locker(LockVariables);
    String name_;
    if(*name != '$')
        name_ = "$";
    name_ += name;
    VariableMap::iterator found = variables.find(name_);
    if(found == variables.end()) //not found
        return false;
    if(found->second.alias.length())
        return vardel(found->second.alias.c_str(), delsystem);
    if(!delsystem && found->second.type != VAR_USER)
        return false;
    found = variables.begin();
    while(found != variables.end())
    {
        VariableMap::iterator del = found;
        found++;
        if(found->second.name == String(name))
            variables.erase(del);
    }
    return true;
}

bool vargettype(const char* name, VAR_TYPE* type, VAR_VALUE_TYPE* valtype)
{
    CriticalSectionLocker locker(LockVariables);
    String name_;
    if(*name != '$')
        name_ = "$";
    name_ += name;
    VariableMap::iterator found = variables.find(name_);
    if(found == variables.end()) //not found
        return false;
    if(found->second.alias.length())
        return vargettype(found->second.alias.c_str(), type, valtype);
    if(valtype)
        *valtype = found->second.value.type;
    if(type)
        *type = found->second.type;
    return true;
}

bool varenum(VAR* entries, size_t* cbsize)
{
    CriticalSectionLocker locker(LockVariables);
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