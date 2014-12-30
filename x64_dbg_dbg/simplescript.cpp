#include "simplescript.h"
#include "value.h"
#include "console.h"
#include "argument.h"
#include "variable.h"
#include "threading.h"
#include "x64_dbg.h"
#include "debugger.h"

static std::vector<LINEMAPENTRY> linemap;
static std::vector<SCRIPTBP> scriptbplist;
static std::vector<int> scriptstack;
static int scriptIp = 0;
static bool volatile bAbort = false;
static bool volatile bIsRunning = false;

static SCRIPTBRANCHTYPE scriptgetbranchtype(const char* text)
{
    char newtext[MAX_SCRIPT_LINE_SIZE] = "";
    strcpy_s(newtext, text);
    argformat(newtext); //format jump commands
    if(!strstr(newtext, " "))
        strcat(newtext, " ");
    if(!strncmp(newtext, "jmp ", 4) or !strncmp(newtext, "goto ", 5))
        return scriptjmp;
    else if(!strncmp(newtext, "jbe ", 4) or !strncmp(newtext, "ifbe ", 5) or !strncmp(newtext, "ifbeq ", 6) or !strncmp(newtext, "jle ", 4) or !strncmp(newtext, "ifle ", 5) or !strncmp(newtext, "ifleq ", 6))
        return scriptjbejle;
    else if(!strncmp(newtext, "jae ", 4) or !strncmp(newtext, "ifae ", 5) or !strncmp(newtext, "ifaeq ", 6) or !strncmp(newtext, "jge ", 4) or !strncmp(newtext, "ifge ", 5) or !strncmp(newtext, "ifgeq ", 6))
        return scriptjaejge;
    else if(!strncmp(newtext, "jne ", 4) or !strncmp(newtext, "ifne ", 5) or !strncmp(newtext, "ifneq ", 6) or !strncmp(newtext, "jnz ", 4) or !strncmp(newtext, "ifnz ", 5))
        return scriptjnejnz;
    else if(!strncmp(newtext, "je ", 3)  or !strncmp(newtext, "ife ", 4) or !strncmp(newtext, "ifeq ", 5) or !strncmp(newtext, "jz ", 3) or !strncmp(newtext, "ifz ", 4))
        return scriptjejz;
    else if(!strncmp(newtext, "jb ", 3) or !strncmp(newtext, "ifb ", 4) or !strncmp(newtext, "jl ", 3) or !strncmp(newtext, "ifl ", 4))
        return scriptjbjl;
    else if(!strncmp(newtext, "ja ", 3) or !strncmp(newtext, "ifa ", 4) or !strncmp(newtext, "jg ", 3) or !strncmp(newtext, "ifg ", 4))
        return scriptjajg;
    else if(!strncmp(newtext, "call ", 5))
        return scriptcall;
    return scriptnobranch;
}

static int scriptlabelfind(const char* labelname)
{
    int linecount = (int)linemap.size();
    for(int i = 0; i < linecount; i++)
        if(linemap.at(i).type == linelabel && !strcmp(linemap.at(i).u.label, labelname))
            return i + 1;
    return 0;
}

static int scriptinternalstep(int fromIp) //internal step routine
{
    int maxIp = (int)linemap.size(); //maximum ip
    if(fromIp >= maxIp) //script end
        return fromIp;
    while((linemap.at(fromIp).type == lineempty or linemap.at(fromIp).type == linecomment or linemap.at(fromIp).type == linelabel) and fromIp < maxIp) //skip empty lines
        fromIp++;
    fromIp++;
    return fromIp;
}

static bool scriptcreatelinemap(const char* filename)
{
    HANDLE hFile = CreateFileW(StringUtils::Utf8ToUtf16(filename).c_str(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if(hFile == INVALID_HANDLE_VALUE)
    {
        GuiScriptError(0, "CreateFile failed...");
        return false;
    }
    unsigned int filesize = GetFileSize(hFile, 0);
    if(!filesize)
    {
        CloseHandle(hFile);
        GuiScriptError(0, "Empty script...");
        return false;
    }
    Memory<char*> filedata(filesize + 1, "createlinemap:filedata");
    memset(filedata, 0, filesize + 1);
    DWORD read = 0;
    if(!ReadFile(hFile, filedata, filesize, &read, 0))
    {
        CloseHandle(hFile);
        GuiScriptError(0, "ReadFile failed...");
        return false;
    }
    CloseHandle(hFile);
    int len = (int)strlen(filedata);
    char temp[256] = "";
    LINEMAPENTRY entry;
    memset(&entry, 0, sizeof(entry));
    std::vector<LINEMAPENTRY>().swap(linemap);
    for(int i = 0, j = 0; i < len; i++) //make raw line map
    {
        if(filedata[i] == '\r' and filedata[i + 1] == '\n') //windows file
        {
            memset(&entry, 0, sizeof(entry));
            int add = 0;
            while(temp[add] == ' ')
                add++;
            strcpy(entry.raw, temp + add);
            *temp = 0;
            j = 0;
            i++;
            linemap.push_back(entry);
        }
        else if(filedata[i] == '\n') //other file
        {
            memset(&entry, 0, sizeof(entry));
            int add = 0;
            while(temp[add] == ' ')
                add++;
            strcpy(entry.raw, temp + add);
            *temp = 0;
            j = 0;
            linemap.push_back(entry);
        }
        else if(j >= 254)
        {
            memset(&entry, 0, sizeof(entry));
            int add = 0;
            while(temp[add] == ' ')
                add++;
            strcpy(entry.raw, temp + add);
            *temp = 0;
            j = 0;
            linemap.push_back(entry);
        }
        else
            j += sprintf(temp + j, "%c", filedata[i]);
    }
    if(*temp)
    {
        memset(&entry, 0, sizeof(entry));
        strcpy(entry.raw, temp);
        linemap.push_back(entry);
    }
    unsigned int linemapsize = (unsigned int)linemap.size();
    while(!*linemap.at(linemapsize - 1).raw) //remove empty lines from the end
    {
        linemapsize--;
        linemap.pop_back();
    }
    for(unsigned int i = 0; i < linemapsize; i++)
    {
        LINEMAPENTRY cur = linemap.at(i);

        //temp. remove comments from the raw line
        char line_comment[256] = "";
        char* comment = strstr(&cur.raw[0], "//");
        if(comment && comment != cur.raw) //only when the line doesnt start with a comment
        {
            if(*(comment - 1) == ' ') //space before comment
            {
                strcpy(line_comment, comment);
                *(comment - 1) = '\0';
            }
            else //no space before comment
            {
                strcpy_s(line_comment, comment);
                *comment = 0;
            }
        }

        int rawlen = (int)strlen(cur.raw);
        if(!rawlen) //empty
        {
            cur.type = lineempty;
        }
        else if(!strncmp(cur.raw, "//", 2)) //comment
        {
            cur.type = linecomment;
            strcpy(cur.u.comment, cur.raw);
        }
        else if(cur.raw[rawlen - 1] == ':') //label
        {
            cur.type = linelabel;
            sprintf(cur.u.label, "l %.*s", rawlen - 1, cur.raw); //create a fake command for formatting
            argformat(cur.u.label); //format labels
            char temp[256] = "";
            strcpy_s(temp, cur.u.label + 2);
            strcpy_s(cur.u.label, temp); //remove fake command
            if(!*cur.u.label or !strcmp(cur.u.label, "\"\"")) //no label text
            {
                char message[256] = "";
                sprintf(message, "Empty label detected on line %d!", i + 1);
                GuiScriptError(0, message);
                std::vector<LINEMAPENTRY>().swap(linemap);
                return false;
            }
            int foundlabel = scriptlabelfind(cur.u.label);
            if(foundlabel) //label defined twice
            {
                char message[256] = "";
                sprintf(message, "Duplicate label \"%s\" detected on lines %d and %d!", cur.u.label, foundlabel, i + 1);
                GuiScriptError(0, message);
                std::vector<LINEMAPENTRY>().swap(linemap);
                return false;
            }
        }
        else if(scriptgetbranchtype(cur.raw) != scriptnobranch) //branch
        {
            cur.type = linebranch;
            cur.u.branch.type = scriptgetbranchtype(cur.raw);
            char newraw[MAX_SCRIPT_LINE_SIZE] = "";
            strcpy(newraw, cur.raw);
            argformat(newraw);
            int len = (int)strlen(newraw);
            for(int i = 0; i < len; i++)
                if(newraw[i] == ' ')
                {
                    strcpy(cur.u.branch.branchlabel, newraw + i + 1);
                    break;
                }
        }
        else
        {
            cur.type = linecommand;
            strcpy(cur.u.command, cur.raw);
        }

        //append the comment to the raw line again
        if(*line_comment)
            sprintf(cur.raw + rawlen, " %s", line_comment);
        linemap.at(i) = cur;
    }
    linemapsize = (int)linemap.size();
    for(unsigned int i = 0; i < linemapsize; i++)
    {
        if(linemap.at(i).type == linebranch) //invalid branch label
        {
            int labelline = scriptlabelfind(linemap.at(i).u.branch.branchlabel);
            if(!labelline) //invalid branch label
            {
                char message[256] = "";
                sprintf(message, "Invalid branch label \"%s\" detected on line %d!", linemap.at(i).u.branch.branchlabel, i + 1);
                GuiScriptError(0, message);
                std::vector<LINEMAPENTRY>().swap(linemap);
                return false;
            }
            else //set the branch destination line
                linemap.at(i).u.branch.dest = scriptinternalstep(labelline);
        }
    }
    if(linemap.at(linemapsize - 1).type == linecomment or linemap.at(linemapsize - 1).type == linelabel) //label/comment on the end
    {
        memset(&entry, 0, sizeof(entry));
        entry.type = linecommand;
        strcpy(entry.raw, "ret");
        strcpy(entry.u.command, "ret");
        linemap.push_back(entry);
    }
    return true;
}

static bool scriptinternalbpget(int line) //internal bpget routine
{
    int bpcount = (int)scriptbplist.size();
    for(int i = 0; i < bpcount; i++)
        if(scriptbplist.at(i).line == line)
            return true;
    return false;
}

static bool scriptinternalbptoggle(int line) //internal breakpoint
{
    if(!line or line > (int)linemap.size()) //invalid line
        return false;
    line = scriptinternalstep(line - 1); //no breakpoints on non-executable locations
    if(scriptinternalbpget(line)) //remove breakpoint
    {
        int bpcount = (int)scriptbplist.size();
        for(int i = 0; i < bpcount; i++)
            if(scriptbplist.at(i).line == line)
            {
                scriptbplist.erase(scriptbplist.begin() + i);
                break;
            }
    }
    else //add breakpoint
    {
        SCRIPTBP newbp;
        newbp.silent = true;
        newbp.line = line;
        scriptbplist.push_back(newbp);
    }
    return true;
}

static bool scriptisruncommand(const char* cmdlist)
{
    if(arraycontains(cmdlist, "run"))
        return true;
    else if(arraycontains(cmdlist, "erun"))
        return true;
    else if(arraycontains(cmdlist, "sti"))
        return true;
    else if(arraycontains(cmdlist, "esti"))
        return true;
    else if(arraycontains(cmdlist, "step"))
        return true;
    else if(arraycontains(cmdlist, "estep"))
        return true;
    else if(arraycontains(cmdlist, "sstep"))
        return true;
    else if(arraycontains(cmdlist, "rtr"))
        return true;
    else if(arraycontains(cmdlist, "ertr"))
        return true;
    return false;
}

static bool scriptisinternalcommand(const char* text, const char* cmd)
{
    int len = (int)strlen(text);
    int cmdlen = (int)strlen(cmd);
    if(cmdlen > len)
        return false;
    else if(cmdlen == len)
        return scmp(text, cmd);
    else if(text[cmdlen] == ' ')
        return (!_strnicmp(text, cmd, cmdlen));
    return false;
}

static CMDRESULT scriptinternalcmdexec(const char* cmd)
{
    if(scriptisinternalcommand(cmd, "ret")) //script finished
    {
        if(!scriptstack.size()) //nothing on the stack
        {
            GuiScriptMessage("Script finished!");
            return STATUS_EXIT;
        }
        scriptIp = scriptstack.back(); //set scriptIp to the call address (scriptinternalstep will step over it)
        scriptstack.pop_back(); //remove last stack entry
        return STATUS_CONTINUE;
    }
    else if(scriptisinternalcommand(cmd, "invalid")) //invalid command for testing
        return STATUS_ERROR;
    else if(scriptisinternalcommand(cmd, "pause")) //pause the script
        return STATUS_PAUSE;
    else if(scriptisinternalcommand(cmd, "nop")) //do nothing
        return STATUS_CONTINUE;
    char command[deflen] = "";
    strcpy_s(command, cmd);
    argformat(command);
    COMMAND* found = cmdfindmain(dbggetcommandlist(), command);
    if(!found) //invalid command
        return STATUS_ERROR;
    if(arraycontains(found->name, "var")) //var
    {
        cmddirectexec(dbggetcommandlist(), command);
        return STATUS_CONTINUE;
    }
    CMDRESULT res = cmddirectexec(dbggetcommandlist(), command);
    while(DbgIsDebugging() && !waitislocked(WAITID_RUN)) //while not locked (NOTE: possible deadlock)
        Sleep(10);
    return res;
}

static bool scriptinternalbranch(SCRIPTBRANCHTYPE type) //determine if we should jump
{
    uint ezflag = 0;
    uint bsflag = 0;
    varget("$_EZ_FLAG", &ezflag, 0, 0);
    varget("$_BS_FLAG", &bsflag, 0, 0);
    bool bJump = false;
    switch(type)
    {
    case scriptcall:
    case scriptjmp:
        bJump = true;
        break;
    case scriptjnejnz: //$_EZ_FLAG=0
        if(!ezflag)
            bJump = true;
        break;
    case scriptjejz: //$_EZ_FLAG=1
        if(ezflag)
            bJump = true;
        break;
    case scriptjbjl: //$_BS_FLAG=0 and $_EZ_FLAG=0 //below, not equal
        if(!bsflag and !ezflag)
            bJump = true;
        break;
    case scriptjajg: //$_BS_FLAG=1 and $_EZ_FLAG=0 //above, not equal
        if(bsflag and !ezflag)
            bJump = true;
        break;
    case scriptjbejle: //$_BS_FLAG=0 or $_EZ_FLAG=1
        if(!bsflag or ezflag)
            bJump = true;
        break;
    case scriptjaejge: //$_BS_FLAG=1 or $_EZ_FLAG=1
        if(bsflag or ezflag)
            bJump = true;
        break;
    default:
        bJump = false;
        break;
    }
    return bJump;
}

static bool scriptinternalcmd()
{
    bool bContinue = true;
    LINEMAPENTRY cur = linemap.at(scriptIp - 1);
    if(cur.type == linecommand)
    {
        switch(scriptinternalcmdexec(cur.u.command))
        {
        case STATUS_CONTINUE:
            break;
        case STATUS_ERROR:
            bContinue = false;
            GuiScriptError(scriptIp, "Error executing command!");
            break;
        case STATUS_EXIT:
            bContinue = false;
            scriptIp = scriptinternalstep(0);
            GuiScriptSetIp(scriptIp);
            break;
        case STATUS_PAUSE:
            bContinue = false; //stop running the script
            scriptIp = scriptinternalstep(scriptIp);
            GuiScriptSetIp(scriptIp);
            break;
        }
    }
    else if(cur.type == linebranch)
    {
        if(cur.u.branch.type == scriptcall) //calls have a special meaning
            scriptstack.push_back(scriptIp);
        if(scriptinternalbranch(cur.u.branch.type))
            scriptIp = scriptlabelfind(cur.u.branch.branchlabel);
    }
    return bContinue;
}

static DWORD WINAPI scriptRunThread(void* arg)
{
    int destline = (int)(uint)arg;
    if(!destline or destline > (int)linemap.size()) //invalid line
        destline = 0;
    if(destline)
    {
        destline = scriptinternalstep(destline - 1); //no breakpoints on non-executable locations
        if(!scriptinternalbpget(destline)) //no breakpoint set
            scriptinternalbptoggle(destline);
    }
    bAbort = false;
    if(scriptIp)
        scriptIp--;
    scriptIp = scriptinternalstep(scriptIp);
    bool bContinue = true;
    while(bContinue && !bAbort) //run loop
    {
        bContinue = scriptinternalcmd();
        if(scriptIp == scriptinternalstep(scriptIp)) //end of script
        {
            bContinue = false;
            scriptIp = scriptinternalstep(0);
        }
        if(bContinue)
            scriptIp = scriptinternalstep(scriptIp); //this is the next ip
        if(scriptinternalbpget(scriptIp)) //breakpoint=stop run loop
            bContinue = false;
        Sleep(1); //don't fry the processor
    }
    bIsRunning = false; //not running anymore
    GuiScriptSetIp(scriptIp);
    return 0;
}

static DWORD WINAPI scriptLoadThread(void* filename)
{
    GuiScriptClear();
    GuiScriptEnableHighlighting(true); //enable default script syntax highlighting
    scriptIp = 0;
    std::vector<SCRIPTBP>().swap(scriptbplist); //clear breakpoints
    std::vector<int>().swap(scriptstack); //clear script stack
    bAbort = false;
    if(!scriptcreatelinemap((const char*)filename))
        return 0;
    int lines = (int)linemap.size();
    const char** script = (const char**)BridgeAlloc(lines * sizeof(const char*));
    for(int i = 0; i < lines; i++) //add script lines
        script[i] = linemap.at(i).raw;
    GuiScriptAdd(lines, script);
    scriptIp = scriptinternalstep(0);
    GuiScriptSetIp(scriptIp);
    return 0;
}

void scriptload(const char* filename)
{
    static char filename_[MAX_PATH] = "";
    strcpy_s(filename_, filename);
    CloseHandle(CreateThread(0, 0, scriptLoadThread, filename_, 0, 0));
}

void scriptunload()
{
    GuiScriptClear();
    scriptIp = 0;
    std::vector<SCRIPTBP>().swap(scriptbplist); //clear breakpoints
    bAbort = false;
}

void scriptrun(int destline)
{
    if(DbgIsDebugging() && !waitislocked(WAITID_RUN))
    {
        GuiScriptError(0, "Debugger must be paused to run a script!");
        return;
    }
    if(bIsRunning) //already running
        return;
    bIsRunning = true;
    CloseHandle(CreateThread(0, 0, scriptRunThread, (void*)(uint)destline, 0, 0));
}

DWORD WINAPI scriptStepThread(void* param)
{
    if(bIsRunning) //already running
        return 0;
    scriptIp = scriptinternalstep(scriptIp - 1); //probably useless
    if(!scriptinternalcmd())
        return 0;
    if(scriptIp == scriptinternalstep(scriptIp)) //end of script
        scriptIp = 0;
    scriptIp = scriptinternalstep(scriptIp);
    GuiScriptSetIp(scriptIp);
    return 0;
}

void scriptstep()
{
    CloseHandle(CreateThread(0, 0, scriptStepThread, 0, 0, 0));
}

bool scriptbptoggle(int line)
{
    if(!line or line > (int)linemap.size()) //invalid line
        return false;
    line = scriptinternalstep(line - 1); //no breakpoints on non-executable locations
    if(scriptbpget(line)) //remove breakpoint
    {
        int bpcount = (int)scriptbplist.size();
        for(int i = 0; i < bpcount; i++)
            if(scriptbplist.at(i).line == line && !scriptbplist.at(i).silent)
            {
                scriptbplist.erase(scriptbplist.begin() + i);
                break;
            }
    }
    else //add breakpoint
    {
        SCRIPTBP newbp;
        newbp.silent = false;
        newbp.line = line;
        scriptbplist.push_back(newbp);
    }
    return true;
}

bool scriptbpget(int line)
{
    int bpcount = (int)scriptbplist.size();
    for(int i = 0; i < bpcount; i++)
        if(scriptbplist.at(i).line == line && !scriptbplist.at(i).silent)
            return true;
    return false;
}

bool scriptcmdexec(const char* command)
{
    switch(scriptinternalcmdexec(command))
    {
    case STATUS_ERROR:
        return false;
        break;
    case STATUS_EXIT:
        scriptIp = scriptinternalstep(0);
        GuiScriptSetIp(scriptIp);
        break;
    case STATUS_PAUSE:
    case STATUS_CONTINUE:
        break;
    }
    return true;
}

void scriptabort()
{
    if(bIsRunning)
    {
        bAbort = true;
        while(bIsRunning)
            Sleep(1);
    }
    else //reset the script
        scriptsetip(0);
}

SCRIPTLINETYPE scriptgetlinetype(int line)
{
    if(line > (int)linemap.size())
        return lineempty;
    return linemap.at(line - 1).type;
}

void scriptsetip(int line)
{
    if(line)
        line--;
    scriptIp = scriptinternalstep(line);
    GuiScriptSetIp(scriptIp);
}

void scriptreset()
{
    while(bIsRunning)
    {
        bAbort = true;
        Sleep(1);
    }
    Sleep(10);
    scriptsetip(0);
}

bool scriptgetbranchinfo(int line, SCRIPTBRANCH* info)
{
    if(!info or !line or line > (int)linemap.size()) //invalid line
        return false;
    if(linemap.at(line - 1).type != linebranch) //no branch
        return false;
    memcpy(info, &linemap.at(line - 1).u.branch, sizeof(SCRIPTBRANCH));
    return true;
}

CMDRESULT cbScriptLoad(int argc, char* argv[])
{
    if(argc < 2)
        return STATUS_ERROR;
    scriptload(argv[1]);
    return STATUS_CONTINUE;
}

CMDRESULT cbScriptMsg(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    GuiScriptMessage(argv[1]);
    return STATUS_CONTINUE;
}

CMDRESULT cbScriptMsgyn(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    varset("$RESULT", GuiScriptMsgyn(argv[1]), false);
    return STATUS_CONTINUE;
}
