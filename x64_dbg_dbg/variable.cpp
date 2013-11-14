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

void varinit()
{
    vars=(VAR*)emalloc(sizeof(VAR));
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
}

VAR* vargetptr()
{
    return vars;
}

bool varnew(const char* name_, uint value, VAR_TYPE type)
{
    if(!name_)
        return false;
    char* name=(char*)emalloc(strlen(name_)+2);
    if(*name_!='$')
    {
        *name='$';
        strcpy(name+1, name_);
    }
    else
        strcpy(name, name_);
    if(!name[1])
    {
        efree(name);
        return false;
    }
    if(varfind(name, 0))
    {
        efree(name);
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
        var=(VAR*)emalloc(sizeof(VAR));
    memset(var, 0, sizeof(VAR));
    var->name=name;
    var->type=type;
    var->value.value=value;
    if(!nonext)
    {
        VAR* cur=vars;
        while(cur->next)
            cur=cur->next;
        cur->next=var;
    }
    return true;
}

bool varget(const char* name, uint* value, int* size, VAR_TYPE* type)
{
    char newname[deflen]="$";
    int add=0;
    if(*name=='$')
        add=1;
    strcat(newname, name+add);
    VAR* found=varfind(newname, 0);
    if(!found)
        return false;
    if(!value)
        return false;
    if(type)
        *type=found->type;
    *value=found->value.value;
    return true;
}

bool varset(const char* name, uint value, bool setreadonly)
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
    found->value.value=value;
    return true;
}

bool vardel(const char* name_, bool delsystem)
{
    char* name=(char*)emalloc(strlen(name_)+2);
    if(*name_!='$')
    {
        *name='$';
        strcpy(name+1, name_);
    }
    else
        strcpy(name, name_);
    VAR* prev=0;
    VAR* found=varfind(name, &prev);
    efree(name);
    if(!found)
        return false;
    VAR_TYPE type=found->type;
    if(!delsystem and type!=VAR_USER)
        return false;
    if(type==VAR_HIDDEN)
        return false;
    efree(found->name);
    if(found==vars)
    {
        VAR* next=vars->next;
        if(next)
        {
            memcpy(vars, vars->next, sizeof(VAR));
            vars->next=next->next;
            efree(next);
        }
        else
            memset(vars, 0, sizeof(VAR));
    }
    else
    {
        prev->next=found->next;
        efree(found);
    }
    return true;
}
