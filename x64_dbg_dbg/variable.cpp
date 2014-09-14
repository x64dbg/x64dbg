/**
 @file variable.cpp

 @brief Implements the variable class.
 */

#include "variable.h"

/**
 @brief The variables.
 */

static VariableMap variables;

/**
 @brief The variables.
 */

static VAR* vars;

/**
 @fn static void varsetvalue(VAR* var, VAR_VALUE* value)

 @brief Varsetvalues.

 @param [in,out] var   If non-null, the variable.
 @param [in,out] value If non-null, the value.
 */

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

/**
 @fn static bool varset(const char* name, VAR_VALUE* value, bool setreadonly)

 @brief Varsets.

 @param name           The name.
 @param [in,out] value If non-null, the value.
 @param setreadonly    true to setreadonly.

 @return true if it succeeds, false if it fails.
 */

static bool varset(const char* name, VAR_VALUE* value, bool setreadonly)
{
    std::string name_;
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

/**
 @fn void varinit()

 @brief Varinits this object.
 */

void varinit()
{
    variables.clear();
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

/**
 @fn void varfree()

 @brief Varfrees this object.
 */

void varfree()
{
    variables.clear();
}

/**
 @fn VAR* vargetptr()

 @brief Gets the vargetptr.

 @return null if it fails, else a VAR*.
 */

VAR* vargetptr()
{
    return 0;
}

#include <iostream>
#include <sstream>

/**
 @fn std::vector<std::string> & split(const std::string & s, char delim, std::vector<std::string> & elems)

 @brief Splits.

 @param s              The const std::string &amp; to process.
 @param delim          The delimiter.
 @param [in,out] elems The elements.

 @return A std::vector&lt;std::string&gt;&amp;
 */

std::vector<std::string> & split(const std::string & s, char delim, std::vector<std::string> & elems)
{
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss, item, delim))
    {
        if(!item.length())
            continue;
        elems.push_back(item);
    }
    return elems;
}

/**
 @fn std::vector<std::string> split(const std::string & s, char delim)

 @brief Splits.

 @param s     The const std::string &amp; to process.
 @param delim The delimiter.

 @return A std::vector&lt;std::string&gt;
 */

std::vector<std::string> split(const std::string & s, char delim)
{
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}

/**
 @fn bool varnew(const char* name, uint value, VAR_TYPE type)

 @brief Varnews.

 @param name  The name.
 @param value The value.
 @param type  The type.

 @return true if it succeeds, false if it fails.
 */

bool varnew(const char* name, uint value, VAR_TYPE type)
{
    if(!name)
        return false;
    std::vector<std::string> names = split(name, '\1');
    std::string firstName;
    for(int i = 0; i < (int)names.size(); i++)
    {
        std::string name_;
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

/**
 @fn static bool varget(const char* name, VAR_VALUE* value, int* size, VAR_TYPE* type)

 @brief Vargets.

 @param name           The name.
 @param [in,out] value If non-null, the value.
 @param [in,out] size  If non-null, the size.
 @param [in,out] type  If non-null, the type.

 @return true if it succeeds, false if it fails.
 */

static bool varget(const char* name, VAR_VALUE* value, int* size, VAR_TYPE* type)
{
    std::string name_;
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

/**
 @fn bool varget(const char* name, uint* value, int* size, VAR_TYPE* type)

 @brief Vargets.

 @param name           The name.
 @param [in,out] value If non-null, the value.
 @param [in,out] size  If non-null, the size.
 @param [in,out] type  If non-null, the type.

 @return true if it succeeds, false if it fails.
 */

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

/**
 @fn bool varget(const char* name, char* string, int* size, VAR_TYPE* type)

 @brief Vargets.

 @param name            The name.
 @param [in,out] string If non-null, the string.
 @param [in,out] size   If non-null, the size.
 @param [in,out] type   If non-null, the type.

 @return true if it succeeds, false if it fails.
 */

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

/**
 @fn bool varset(const char* name, uint value, bool setreadonly)

 @brief Varsets.

 @param name        The name.
 @param value       The value.
 @param setreadonly true to setreadonly.

 @return true if it succeeds, false if it fails.
 */

bool varset(const char* name, uint value, bool setreadonly)
{
    VAR_VALUE varvalue;
    varvalue.size = sizeof(uint);
    varvalue.type = VAR_UINT;
    varvalue.u.value = value;
    return varset(name, &varvalue, setreadonly);
}

/**
 @fn bool varset(const char* name, const char* string, bool setreadonly)

 @brief Varsets.

 @param name        The name.
 @param string      The string.
 @param setreadonly true to setreadonly.

 @return true if it succeeds, false if it fails.
 */

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

/**
 @fn bool vardel(const char* name, bool delsystem)

 @brief Vardels.

 @param name      The name.
 @param delsystem true to delsystem.

 @return true if it succeeds, false if it fails.
 */

bool vardel(const char* name, bool delsystem)
{
    std::string name_;
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
        if(found->second.name == std::string(name))
            variables.erase(del);
    }
    return true;
}

/**
 @fn bool vargettype(const char* name, VAR_TYPE* type, VAR_VALUE_TYPE* valtype)

 @brief Vargettypes.

 @param name             The name.
 @param [in,out] type    If non-null, the type.
 @param [in,out] valtype If non-null, the valtype.

 @return true if it succeeds, false if it fails.
 */

bool vargettype(const char* name, VAR_TYPE* type, VAR_VALUE_TYPE* valtype)
{
    std::string name_;
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

/**
 @fn bool varenum(VAR* entries, size_t* cbsize)

 @brief Varenums.

 @param [in,out] entries If non-null, the entries.
 @param [in,out] cbsize  If non-null, the cbsize.

 @return true if it succeeds, false if it fails.
 */

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