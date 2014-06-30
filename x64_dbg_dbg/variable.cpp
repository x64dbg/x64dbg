#include "variable.h"

static VAR* vars;

static VAR* varfind(const char* name, VAR** link)
{
    VAR* cur=vars;
    if(!cur)
        return 0;
    VAR* prev=0;
    while(cur)
    {
        if(arraycontains(cur->name, name))
        {
            if(link)
                *link=prev;
            return cur;
        }
        prev=cur;
        cur=cur->next;
    }
    return 0;
}

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
    char newname[deflen]="$";
    int add=0;
    if(*name=='$')
        add=1;
    strcat(newname, name+add);
    VAR* found=varfind(newname, 0);
    if(!found)
        return false;
    if(!setreadonly and (found->type==VAR_READONLY or found->type==VAR_HIDDEN))
        return false;
    varsetvalue(found, value);
    return true;
}

void varinit()
{
    vars=(VAR*)emalloc(sizeof(VAR), "varinit:vars");
    memset(vars, 0, sizeof(VAR));
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
    VAR* cur=vars;
    while(cur)
    {
        efree(cur->name, "varfree:cur->name");
        VAR* next=cur->next;
        efree(cur, "varfree:cur");
        cur=next;
    }
}

VAR* vargetptr()
{
    return vars;
}

bool varnew(const char* name_, uint value, VAR_TYPE type)
{
    if(!name_)
        return false;
    char* name=(char*)emalloc(strlen(name_)+2, "varnew:name");
    if(*name_!='$')
    {
        *name='$';
        strcpy(name+1, name_);
    }
    else
        strcpy(name, name_);
    if(!name[1])
    {
        efree(name, "varnew:name");
        return false;
    }
    if(varfind(name, 0))
    {
        efree(name, "varnew:name");
        return false;
    }
    VAR* var;
    bool nonext=false;
    if(!vars->name)
    {
        nonext=true;
        var=vars;
    }
    else
        var=(VAR*)emalloc(sizeof(VAR), "varnew:var");
    memset(var, 0, sizeof(VAR));
    var->name=name;
    var->type=type;
    VAR_VALUE varvalue;
    varvalue.size=sizeof(uint);
    varvalue.type=VAR_UINT;
    varvalue.u.value=value;
    varsetvalue(var, &varvalue);
    if(!nonext)
    {
        VAR* cur=vars;
        while(cur->next)
            cur=cur->next;
        cur->next=var;
    }
    return true;
}

static bool varget(const char* name, VAR_VALUE* value, int* size, VAR_TYPE* type)
{
    char newname[deflen]="$";
    int add=0;
    if(*name=='$')
        add=1;
    strcat(newname, name+add);
    VAR* found=varfind(newname, 0);
    if(!found or !value or !size or !type)
        return false;
    *type=found->type;
    *size=found->value.size;
    memcpy(value, &found->value, sizeof(VAR_VALUE));
    return true;
}

bool varget(const char* name, uint* value, int* size, VAR_TYPE* type)
{
    VAR_VALUE varvalue;
    int varsize;
    VAR_TYPE vartype;
    if(!varget(name, &varvalue, &varsize, &vartype) or varvalue.type!=VAR_UINT)
        return false;
    if(size)
        *size=varsize;
    if(!value && size)
        return true; //variable was valid, just get the size
    if(type)
        *type=vartype;
    *value=varvalue.u.value;
    return true;
}

bool varget(const char* name, char* string, int* size, VAR_TYPE* type)
{
    VAR_VALUE varvalue;
    int varsize;
    VAR_TYPE vartype;
    if(!varget(name, &varvalue, &varsize, &vartype) or varvalue.type!=VAR_STRING)
        return false;
    if(size)
        *size=varsize;
    if(!string && size)
        return true; //variable was valid, just get the size
    if(type)
        *type=vartype;
    memcpy(string, &varvalue.u.data->front(), varsize);
    return true;
}

bool varset(const char* name, uint value, bool setreadonly)
{
    VAR_VALUE varvalue;
    varvalue.size=sizeof(uint);
    varvalue.type=VAR_UINT;
    varvalue.u.value=value;
    varset(name, &varvalue, setreadonly);
    return true;
}

bool varset(const char* name, const char* string, bool setreadonly)
{
    VAR_VALUE varvalue;
    int size=strlen(string);
    varvalue.size=size;
    varvalue.type=VAR_STRING;
    varvalue.u.data=new std::vector<unsigned char>;
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
    char* name_=(char*)emalloc(strlen(name)+2, "vardel:name");
    if(*name!='$')
    {
        *name_='$';
        strcpy(name_+1, name);
    }
    else
        strcpy(name_, name);
    VAR* prev=0;
    VAR* found=varfind(name_, &prev);
    efree(name_, "vardel:name");
    if(!found)
        return false;
    VAR_TYPE type=found->type;
    if(!delsystem and type!=VAR_USER)
        return false;
    if(type==VAR_HIDDEN)
        return false;
    VAR_VALUE varvalue;
    varvalue.size=sizeof(uint);
    varvalue.type=VAR_UINT;
    varvalue.u.value=0;
    varsetvalue(found, &varvalue);
    efree(found->name, "vardel:found->name");
    if(found==vars)
    {
        VAR* next=vars->next;
        if(next)
        {
            memcpy(vars, vars->next, sizeof(VAR));
            vars->next=next->next;
            efree(next, "vardel:next");
        }
        else
            memset(vars, 0, sizeof(VAR));
    }
    else
    {
        prev->next=found->next;
        efree(found, "vardel:found");
    }
    return true;
}

bool vargettype(const char* name, VAR_TYPE* type, VAR_VALUE_TYPE* valtype)
{
    char newname[deflen]="$";
    int add=0;
    if(*name=='$')
        add=1;
    strcat(newname, name+add);
    VAR* found=varfind(newname, 0);
    if(!found)
        return false;
    if(valtype)
        *valtype=found->value.type;
    if(type)
        *type=found->type;
    return true;
}
