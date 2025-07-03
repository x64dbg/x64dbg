==============================
_plugin_registerexprfunctionex
==============================
This function registers an expression function defined by a plugin, so that users can use it in expressions. Unlike :doc:`registerexprfunction`, this function can also register expression functions that process string argument types.

::

    bool _plugin_registerexprfunctionex(
        int pluginHandle,                  //plugin handle
        const char* name,                  //name of expresison function
        const ValueType & returnType,      //type of return value
        const ValueType* argTypes,         //type of arguments
        size_t argc,                       //number of arguments
        CBPLUGINEXPRFUNCTIONEX cbFunction, //callback function
        void* userdata                     //user data
    ); 

----------
Parameters
----------

:pluginHandle: Handle of the calling plugin. 
:name: Name of expresison function.
:returnType: Type of return value. The definition of ValueType is:

::

    typedef enum
    {
        ValueTypeNumber,
        ValueTypeString,
        // Types below cannot be used for values, only for registration
        ValueTypeAny,
        ValueTypeOptionalNumber,
        ValueTypeOptionalString,
        ValueTypeOptionalAny,
    } ValueType;

:argTypes: Array of types of arguments
:argc: Number of arguments of expression function.
:cbFunction: Callback with the following typdef:

::

    typedef struct
    {
        const char* ptr; // Should be allocated with BridgeAlloc
        bool isOwner; // When set to true BridgeFree will be called on ptr
    } StringValue;

    typedef struct
    {
        ValueType type;
        duint number;
        StringValue string;
    } ExpressionValue;

    typedef bool(*CBPLUGINEXPRFUNCTIONEX)(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata);

:userdata: A pointer value passed to the callback, may be used by plugin to pass additional information.

-------------
Return Values
-------------
Return true when the registration is successful, otherwise return false.
