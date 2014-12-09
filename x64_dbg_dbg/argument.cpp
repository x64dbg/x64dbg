#include "argument.h"
#include "console.h"

/*
formatarg:
01) remove prepended spaces
02) get command (first space) and lowercase
03) get arguments
04) remove double quotes (from arguments)
05) temp. remove double backslash
06) remove prepended/appended non-escaped commas and spaces (from arguments)
    a) prepended
    b) appended
07) get quote count, ignore escaped (from arguments)
08) process quotes (from arguments):
    a) zero quotes
    b) restore double backslash
    c) escape commas and spaces
09) temp. remove double backslash
10) remove unescaped double commas (from arguments)
11) remove unescaped spaces (from arguments)
12) restore double backslash
13) combine formatted arguments and command
*/
void argformat(char* cmd)
{
    if(strlen(cmd) >= deflen)
        return;

    char command_[deflen] = "";
    char* command = command_;
    strcpy(command, cmd);
    while(*command == ' ')
        command++;

    int len = (int)strlen(command);
    int start = 0;
    for(int i = 0; i < len; i++)
        if(command[i] == ' ')
        {
            command[i] = 0;
            start = i + 1;
            break;
        }
    if(!start)
        start = len;

    char arguments_[deflen] = "";
    char* arguments = arguments_;
    strcpy_s(arguments, deflen, command + start);

    char temp[deflen] = "";
    len = (int)strlen(arguments);
    for(int i = 0, j = 0; i < len; i++)
    {
        if(arguments[i] == '"' and arguments[i + 1] == '"') //TODO: fix this
            i += 2;
        j += sprintf(temp + j, "%c", arguments[i]);
    }
    strcpy_s(arguments, deflen, temp);

    len = (int)strlen(arguments);
    for(int i = 0; i < len; i++)
        if(arguments[i] == '\\' and arguments[i + 1] == '\\')
        {
            arguments[i] = 1;
            arguments[i + 1] = 1;
        }

    while((*arguments == ',' or * arguments == ' ') and * (arguments - 1) != '\\')
        arguments++;
    len = (int)strlen(arguments);
    while((arguments[len - 1] == ' ' or arguments[len - 1] == ',') and arguments[len - 2] != '\\')
        len--;
    arguments[len] = 0;

    len = (int)strlen(arguments);
    int quote_count = 0;
    for(int i = 0; i < len; i++)
        if(arguments[i] == '"')
            quote_count++;

    if(!(quote_count % 2))
    {
        for(int i = 0; i < len; i++)
            if(arguments[i] == '"')
                arguments[i] = 0;

        for(int i = 0; i < len; i++)
            if(arguments[i] == 1 and (i < len - 1 and arguments[i + 1] == 1))
            {
                arguments[i] = '\\';
                arguments[i + 1] = '\\';
            }

        for(int i = 0, j = 0; i < len; i++)
        {
            if(!arguments[i])
            {
                i++;
                int len2 = (int)strlen(arguments + i);
                for(int k = 0; k < len2; k++)
                {
                    if(arguments[i + k] == ',' or arguments[i + k] == ' ' or arguments[i + k] == '\\')
                        j += sprintf(temp + j, "\\%c", arguments[i + k]);
                    else
                        j += sprintf(temp + j, "%c", arguments[i + k]);
                }
                i += len2;
            }
            else
                j += sprintf(temp + j, "%c", arguments[i]);
        }
        arguments = arguments_;
        strcpy(arguments, temp);
    }
    len = (int)strlen(arguments);
    for(int i = 0; i < len; i++)
        if(arguments[i] == '\\' and arguments[i + 1] == '\\')
        {
            arguments[i] = 1;
            arguments[i + 1] = 1;
        }
    len = (int)strlen(arguments);
    for(int i = 0, j = 0; i < len; i++)
    {
        if(arguments[i] == ',' and arguments[i + 1] == ',')
            i += 2;
        j += sprintf(temp + j, "%c", arguments[i]);
    }
    strcpy(arguments, temp);

    len = (int)strlen(arguments);
    for(int i = 0, j = 0; i < len; i++)
    {
        while(arguments[i] == ' ' and arguments[i - 1] != '\\')
            i++;
        j += sprintf(temp + j, "%c", arguments[i]);
    }
    strcpy(arguments, temp);

    len = (int)strlen(arguments);
    for(int i = 0; i < len; i++)
        if(arguments[i] == 1 and arguments[i + 1] == 1)
        {
            arguments[i] = '\\';
            arguments[i + 1] = '\\';
        }

    if(strlen(arguments))
        sprintf(cmd, "%s %s", command, arguments);
    else
        strcpy(cmd, command);
}

/*
1) remove double backslash
2) count unescaped commas
*/
int arggetcount(const char* cmd)
{
    int len = (int)strlen(cmd);
    if(!len or len >= deflen)
        return -1;

    int arg_count = 0;

    int start = 0;
    while(cmd[start] != ' ' and start < len)
        start++;
    if(start == len)
        return arg_count;
    arg_count = 1;
    char temp_[deflen] = "";
    char* temp = temp_ + 1;
    strcpy(temp, cmd);
    for(int i = start; i < len; i++)
        if(temp[i] == '\\' and (i < len - 1 and temp[i + 1] == '\\'))
        {
            temp[i] = 1;
            temp[i + 1] = 1;
        }

    for(int i = start; i < len; i++)
    {
        if(temp[i] == ',' and temp[i - 1] != '\\')
            arg_count++;
    }
    return arg_count;
}
/*
1) get arg count
2) remove double backslash
3) zero non-escaped commas
4) restore double backslash
5) handle escape characters
*/
bool argget(const char* cmd, char* arg, int arg_num, bool optional)
{
    if(strlen(cmd) >= deflen)
        return false;
    int argcount = arggetcount(cmd);
    if((arg_num + 1) > argcount)
    {
        if(!optional)
            dprintf("missing argument nr %d\n", arg_num + 1);
        return false;
    }
    int start = 0;
    while(cmd[start] != ' ') //ignore the command
        start++;
    while(cmd[start] == ' ') //ignore initial spaces
        start++;
    char temp_[deflen] = "";
    char* temp = temp_ + 1;
    strcpy(temp, cmd + start);

    int len = (int)strlen(temp);
    for(int i = 0; i < len; i++)
        if(temp[i] == '\\' and temp[i + 1] == '\\')
        {
            temp[i] = 1;
            temp[i + 1] = 1;
        }

    for(int i = 0; i < len; i++)
    {
        if(temp[i] == ',' and temp[i - 1] != '\\')
            temp[i] = 0;
    }

    for(int i = 0; i < len; i++)
        if(temp[i] == 1 and temp[i + 1] == 1)
        {
            temp[i] = '\\';
            temp[i + 1] = '\\';
        }

    char new_temp[deflen] = "";
    int new_len = len;
    for(int i = 0, j = 0; i < len; i++) //handle escape characters
    {
        if(temp[i] == '\\' and (temp[i + 1] == ',' or temp[i + 1] == ' ' or temp[i + 1] == '\\'))
        {
            new_len--;
            j += sprintf(new_temp + j, "%c", temp[i + 1]);
            i++;
        }
        else
            j += sprintf(new_temp + j, "%c", temp[i]);
    }
    len = new_len;
    memcpy(temp, new_temp, len + 1);
    if(arg_num == 0) //first argument
    {
        strcpy(arg, temp);
        return true;
    }
    for(int i = 0, j = 0; i < len; i++)
    {
        if(!temp[i])
            j++;
        if(j == arg_num)
        {
            strcpy(arg, temp + i + 1);
            return true;
        }
    }
    return false;
}
