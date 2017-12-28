/**
 @file variable.cpp

 @brief Implements the variable class.
 */

#include "variable.h"
#include "threading.h"
#include <map>

/**
\brief The container that stores all variables.
*/
std::map<String, VAR, CaseInsensitiveCompare> variables;

/**
\brief Sets a variable with a value.
\param [in,out] Var The variable to set the value of. The previous value will be freed. Cannot be null.
\param [in] Value The new value. Cannot be null.
*/
void varsetvalue(VAR* Var, VAR_VALUE* Value)
{
    // VAR_STRING needs to be freed before destroying it
    if(Var->value.type == VAR_STRING)
        delete Var->value.u.data;

    // Replace all information in the struct
    memcpy(&Var->value, Value, sizeof(VAR_VALUE));
}

/**
\brief Sets a variable by name.
\param Name The name of the variable. Cannot be null.
\param Value The new value. Cannot be null.
\param ReadOnly true to set read-only variables (like $hProcess etc.).
\return true if the variable was set correctly, false otherwise.
*/
bool varset(const char* Name, VAR_VALUE* Value, bool ReadOnly)
{
    EXCLUSIVE_ACQUIRE(LockVariables);

    String name_;
    if(*Name != '$')
        name_ = "$";
    name_ += Name;
    auto found = variables.find(name_);
    if(found == variables.end()) //not found
        return false;
    if(found->second.alias.length())
        return varset(found->second.alias.c_str(), Value, ReadOnly);

    if(!ReadOnly && (found->second.type == VAR_READONLY || found->second.type == VAR_HIDDEN))
        return false;
    varsetvalue(&found->second, Value);
    return true;
}

/**
\brief Initializes various default variables.
*/
void varinit()
{
    varfree();

    // General variables
    varnew("$result\1$res", 0, VAR_SYSTEM);
    varnew("$result1\1$res1", 0, VAR_SYSTEM);
    varnew("$result2\1$res2", 0, VAR_SYSTEM);
    varnew("$result3\1$res3", 0, VAR_SYSTEM);
    varnew("$result4\1$res4", 0, VAR_SYSTEM);
    varnew("$__disasm_refindex", 0, VAR_SYSTEM);
    varnew("$__dump_refindex", 0, VAR_SYSTEM);

    // InitDebug variables
    varnew("$hProcess\1$hp", 0, VAR_READONLY);  // Process handle
    varnew("$pid", 0, VAR_READONLY);            // Process ID

    // Hidden variables
    varnew("$ans\1$an", 0, VAR_HIDDEN);

    // Breakpoint variables
    varnew("$breakpointcounter", 0, VAR_READONLY);
    varnew("$breakpointcondition", 0, VAR_SYSTEM);
    varnew("$breakpointlogcondition", 0, VAR_READONLY);
    varnew("$breakpointexceptionaddress", 0, VAR_READONLY);

    // Tracing variables
    varnew("$tracecounter", 0, VAR_READONLY);
    varnew("$tracecondition", 0, VAR_SYSTEM);
    varnew("$tracelogcondition", 0, VAR_READONLY);
    varnew("$traceswitchcondition", 0, VAR_SYSTEM);

    // Read-only variables
    varnew("$lastalloc", 0, VAR_READONLY);  // Last memory allocation
    varnew("$_EZ_FLAG", 0, VAR_READONLY);   // Equal/zero flag for internal use (1 = equal, 0 = unequal)
    varnew("$_BS_FLAG", 0, VAR_READONLY);   // Bigger/smaller flag for internal use (1 = bigger, 0 = smaller)
}

/**
\brief Clears all variables.
*/
void varfree()
{
    EXCLUSIVE_ACQUIRE(LockVariables);

    // Each variable must be deleted manually; strings especially
    // because there are sub-allocations
    VAR_VALUE emptyValue;
    memset(&emptyValue, 0, sizeof(VAR_VALUE));

    for(auto & itr : variables)
        varsetvalue(&itr.second, &emptyValue);

    // Now clear all vector elements
    variables.clear();
}

/**
\brief Creates a new variable.
\param Name The name of the variable. You can specify alias names by separating the names by '\1'. Cannot be null.
\param Value The new variable value.
\param Type The variable type.
\return true if the new variables was created and set successfully, false otherwise.
*/
bool varnew(const char* Name, duint Value, VAR_TYPE Type)
{
    if(!Name)
        return false;

    EXCLUSIVE_ACQUIRE(LockVariables);

    std::vector<String> names = StringUtils::Split(Name, '\1');
    String firstName;
    for(int i = 0; i < (int)names.size(); i++)
    {
        String name_;
        Name = names.at(i).c_str();
        if(*Name == '$')
        {
            name_ = Name;
        }
        else
        {
            name_ = "$";
            name_ += Name;
        }
        if(!i)
            firstName = name_;
        if(variables.find(name_) != variables.end()) //found
            return false;
        VAR var;
        var.name = name_;
        if(i)
            var.alias = firstName;
        var.type = Type;
        var.value.size = sizeof(duint);
        var.value.type = VAR_UINT;
        var.value.u.value = Value;
        variables.insert(std::make_pair(name_, var));
    }
    return true;
}

/**
\brief Gets a variable value.
\param Name The name of the variable.
\param [out] Value This function can get the variable value. If this value is null, it is ignored.
\param [out] Size This function can get the variable size. If this value is null, it is ignored.
\param [out] Type This function can get the variable type. If this value is null, it is ignored.
\return true if the variable was found and the optional values were retrieved successfully, false otherwise.
*/
bool varget(const char* Name, VAR_VALUE* Value, int* Size, VAR_TYPE* Type)
{
    SHARED_ACQUIRE(LockVariables);

    String name_;
    if(*Name == '$')
    {
        name_ = Name;
    }
    else
    {
        name_ = "$";
        name_ += Name;
    }
    auto found = variables.find(name_);
    if(found == variables.end()) //not found
        return false;
    if(found->second.alias.length())
        return varget(found->second.alias.c_str(), Value, Size, Type);
    if(Type)
        *Type = found->second.type;
    if(Size)
        *Size = found->second.value.size;
    if(Value)
        *Value = found->second.value;
    return true;
}

/**
\brief Gets a variable value.
\param Name The name of the variable.
\param [out] Value This function can get the variable value. If this value is null, it is ignored.
\param [out] Size This function can get the variable size. If this value is null, it is ignored.
\param [out] Type This function can get the variable type. If this value is null, it is ignored.
\return true if the variable was found and the optional values were retrieved successfully, false otherwise.
*/
bool varget(const char* Name, duint* Value, int* Size, VAR_TYPE* Type)
{
    VAR_VALUE varvalue;
    int varsize;
    VAR_TYPE vartype;
    if(!varget(Name, &varvalue, &varsize, &vartype) || varvalue.type != VAR_UINT)
        return false;
    if(Size)
        *Size = varsize;
    if(Type)
        *Type = vartype;
    if(Value)
        *Value = varvalue.u.value;
    return true;
}

/**
\brief Gets a variable value.
\param Name The name of the variable.
\param [out] String This function can get the variable value. If this value is null, it is ignored.
\param [out] Size This function can get the variable size. If this value is null, it is ignored.
\param [out] Type This function can get the variable type. If this value is null, it is ignored.
\return true if the variable was found and the optional values were retrieved successfully, false otherwise.
*/
bool varget(const char* Name, char* String, int* Size, VAR_TYPE* Type)
{
    VAR_VALUE varvalue;
    int varsize;
    VAR_TYPE vartype;
    if(!varget(Name, &varvalue, &varsize, &vartype) || varvalue.type != VAR_STRING)
        return false;
    if(Size)
        *Size = varsize;
    if(Type)
        *Type = vartype;
    if(String)
        memcpy(String, varvalue.u.data->data(), Size ? min(*Size, varsize) : varsize);
    return true;
}

/**
\brief Sets a variable by name.
\param Name The name of the variable. Cannot be null.
\param Value The new value.
\param ReadOnly true to set read-only variables (like $hProcess etc.).
\return true if the variable was set successfully, false otherwise.
*/
bool varset(const char* Name, duint Value, bool ReadOnly)
{
    // Insert variable as an unsigned integer
    VAR_VALUE varValue;
    varValue.size = sizeof(duint);
    varValue.type = VAR_UINT;
    varValue.u.value = Value;

    return varset(Name, &varValue, ReadOnly);
}

/**
\brief Sets a variable by name.
\param Name The name of the variable. Cannot be null.
\param Value The new value. Cannot be null.
\param ReadOnly true to set read-only variables (like $hProcess etc.).
\return true if the variable was set successfully, false otherwise.
*/
bool varset(const char* Name, const char* Value, bool ReadOnly)
{
    VAR_VALUE varValue;
    int stringLen = (int)strlen(Value);
    varValue.size = stringLen;
    varValue.type = VAR_STRING;
    varValue.u.data = new std::vector<unsigned char>;

    // Allocate space for the string
    varValue.u.data->resize(stringLen);

    // Copy directly to vector array
    memcpy(varValue.u.data->data(), Value, stringLen);

    // Try to register variable
    if(!varset(Name, &varValue, ReadOnly))
    {
        delete varValue.u.data;
        return false;
    }

    return true;
}

/**
\brief Deletes a variable.
\param Name The name of the variable to delete. Cannot be null.
\param DelSystem true to allow deleting system variables.
\return true if the variable was deleted successfully, false otherwise.
*/
bool vardel(const char* Name, bool DelSystem)
{
    EXCLUSIVE_ACQUIRE(LockVariables);

    String name_;
    if(*Name != '$')
        name_ = "$";
    name_ += Name;
    auto found = variables.find(name_);
    if(found == variables.end()) //not found
        return false;
    if(found->second.alias.length())
    {
        // Release the lock (potential deadlock here)
        EXCLUSIVE_RELEASE();

        return vardel(found->second.alias.c_str(), DelSystem);
    }

    if(!DelSystem && found->second.type != VAR_USER)
        return false;
    found = variables.begin();
    String NameString(Name);
    while(found != variables.end())
    {
        if(found->first == NameString || found->second.alias == NameString)
        {
            found = variables.erase(found); // Invalidate iterators
        }
        else
            found++;
    }
    return true;
}

/**
\brief Gets a variable type.
\param Name The name of the variable. Cannot be null.
\param [out] Type This function can retrieve the variable type. If null it is ignored.
\param [out] ValueType This function can retrieve the variable value type. If null it is ignored.
\return true if getting the type was successful, false otherwise.
*/
bool vargettype(const char* Name, VAR_TYPE* Type, VAR_VALUE_TYPE* ValueType)
{
    SHARED_ACQUIRE(LockVariables);

    String name_;
    if(*Name != '$')
        name_ = "$";
    name_ += Name;
    auto found = variables.find(name_);
    if(found == variables.end()) //not found
        return false;
    if(found->second.alias.length())
        return vargettype(found->second.alias.c_str(), Type, ValueType);
    if(ValueType)
        *ValueType = found->second.value.type;
    if(Type)
        *Type = found->second.type;
    return true;
}

/**
\brief Enumerates all variables.
\param [in,out] List A pointer to place the variables in. If null, \p cbsize will be filled to the number of bytes required.
\param [in,out] Size This function retrieves the number of bytes required to store all variables. Can be null if \p entries is not null.
\return true if it succeeds, false if it fails.
*/
bool varenum(VAR* List, size_t* Size)
{
    // A list or size must be requested
    if(!List && !Size)
        return false;

    SHARED_ACQUIRE(LockVariables);

    if(Size)
    {
        // Size requested, so return it
        *Size = variables.size() * sizeof(VAR);

        if(!List)
            return true;
    }

    // Fill out all list entries
    for(auto & itr : variables)
    {
        *List = itr.second;
        List++;
    }

    return true;
}
