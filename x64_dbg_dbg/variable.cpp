/**
 @file variable.cpp

 @brief Implements the variable class.
 */

#include "variable.h"
#include "threading.h"

/**
\brief The container that stores the variables.
*/
static VariableMap variables;

/**
\brief Sets a variable with a value.
\param [in,out] var The variable to set the value of. The previous value will be freed. Cannot be null.
\param [in] value The new value. Cannot be null.
*/
static void varsetvalue(VAR* var, VAR_VALUE* value)
{
    // VAR_STRING needs to be freed before destroying it
    if(var->value.type == VAR_STRING)
    {
        var->value.u.data->clear();
        delete var->value.u.data;
    }

    // Replace all information in the struct
    memcpy(&var->value, value, sizeof(VAR_VALUE));
}

/**
\brief Sets a variable by name.
\param name The name of the variable. Cannot be null.
\param value The new value. Cannot be null.
\param setreadonly true to set read-only variables (like $hProcess etc.).
\return true if the variable was set correctly, false otherwise.
*/
static bool varset(const char* name, VAR_VALUE* value, bool setreadonly)
{
    EXCLUSIVE_ACQUIRE(LockVariables);

    String name_;
    if(*name != '$')
        name_ = "$";
    name_ += name;
    VariableMap::iterator found = variables.find(name_);
    if(found == variables.end())  //not found
        return false;
    if(found->second.alias.length())
    {
        // Release the lock (potential deadlock here)
        EXCLUSIVE_RELEASE();

        return varset(found->second.alias.c_str(), value, setreadonly);
    }

    if(!setreadonly && (found->second.type == VAR_READONLY || found->second.type == VAR_HIDDEN))
        return false;
    varsetvalue(&found->second, value);
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

    // InitDebug variables
    varnew("$hProcess\1$hp", 0, VAR_READONLY);  // Process handle
    varnew("$pid", 0, VAR_READONLY);            // Process ID

    // Hidden variables
    varnew("$ans\1$an", 0, VAR_HIDDEN);

    // Read-only variables
    varnew("$lastalloc", 0, VAR_READONLY);  // Last memory allocation
    varnew("$_EZ_FLAG", 0, VAR_READONLY);   // Equal/zero flag for internal use (1=equal, 0=unequal)
    varnew("$_BS_FLAG", 0, VAR_READONLY);   // Bigger/smaller flag for internal use (1=bigger, 0=smaller)
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

	for (auto& itr : variables)
		varsetvalue(&itr.second, &emptyValue);

	// Now clear all vector elements
    variables.clear();
}

/**
\brief Creates a new variable.
\param name The name of the variable. You can specify alias names by separating the names by '\1'. Cannot be null.
\param value The new variable value.
\param type The variable type.
\return true if the new variables was created and set successfully, false otherwise.
*/
bool varnew(const char* name, uint value, VAR_TYPE type)
{
    if(!name)
        return false;

    CriticalSectionLocker locker(LockVariables);

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

/**
\brief Gets a variable value.
\param name The name of the variable.
\param [out] value This function can get the variable value. If this value is null, it is ignored.
\param [out] size This function can get the variable size. If this value is null, it is ignored.
\param [out] type This function can get the variable type. If this value is null, it is ignored.
\return true if the variable was found and the optional values were retrieved successfully, false otherwise.
*/
static bool varget(const char* name, VAR_VALUE* value, int* size, VAR_TYPE* type)
{
    SHARED_ACQUIRE(LockVariables);

    String name_;
    if(*name != '$')
        name_ = "$";
    name_ += name;
    VariableMap::iterator found = variables.find(name_);
    if(found == variables.end()) //not found
        return false;
    if(found->second.alias.length())
    {
        // Release the lock (potential deadlock here)
        SHARED_RELEASE();

        return varget(found->second.alias.c_str(), value, size, type);
    }
    if(type)
        *type = found->second.type;
    if(size)
        *size = found->second.value.size;
    if(value)
        *value = found->second.value;
    return true;
}

/**
\brief Gets a variable value.
\param name The name of the variable.
\param [out] value This function can get the variable value. If this value is null, it is ignored.
\param [out] size This function can get the variable size. If this value is null, it is ignored.
\param [out] type This function can get the variable type. If this value is null, it is ignored.
\return true if the variable was found and the optional values were retrieved successfully, false otherwise.
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
\brief Gets a variable value.
\param name The name of the variable.
\param [out] string This function can get the variable value. If this value is null, it is ignored.
\param [out] size This function can get the variable size. If this value is null, it is ignored.
\param [out] type This function can get the variable type. If this value is null, it is ignored.
\return true if the variable was found and the optional values were retrieved successfully, false otherwise.
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
        memcpy(string, varvalue.u.data->data(), varsize);
    return true;
}

/**
\brief Sets a variable by name.
\param name The name of the variable. Cannot be null.
\param value The new value.
\param setreadonly true to set read-only variables (like $hProcess etc.).
\return true if the variable was set successfully, false otherwise.
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
\brief Sets a variable by name.
\param name The name of the variable. Cannot be null.
\param string The new value. Cannot be null.
\param setreadonly true to set read-only variables (like $hProcess etc.).
\return true if the variable was set successfully, false otherwise.
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
\brief Deletes a variable.
\param name The name of the variable to delete. Cannot be null.
\param delsystem true to allow deleting system variables.
\return true if the variable was deleted successfully, false otherwise.
*/
bool vardel(const char* name, bool delsystem)
{
    EXCLUSIVE_ACQUIRE(LockVariables);

    String name_;
    if(*name != '$')
        name_ = "$";
    name_ += name;
    VariableMap::iterator found = variables.find(name_);
    if(found == variables.end()) //not found
        return false;
    if(found->second.alias.length())
    {
        // Release the lock (potential deadlock here)
        EXCLUSIVE_RELEASE();

        return vardel(found->second.alias.c_str(), delsystem);
    }

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

/**
\brief Gets a variable type.
\param name The name of the variable. Cannot be null.
\param [out] type This function can retrieve the variable type. If null it is ignored.
\param [out] valtype This function can retrieve the variable value type. If null it is ignored.
\return true if getting the type was successful, false otherwise.
*/
bool vargettype(const char* name, VAR_TYPE* type, VAR_VALUE_TYPE* valtype)
{
    SHARED_ACQUIRE(LockVariables);

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

/**
\brief Enumerates all variables.
\param [in,out] entries A pointer to place the variables in. If null, \p cbsize will be filled to the number of bytes required.
\param [in,out] cbsize This function retrieves the number of bytes required to store all variables. Can be null if \p entries is not null.
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