/**
 @file simplescript.cpp

 @brief Implements the simplescript class.
 */

#include "simplescript.h"
#include "console.h"
#include "variable.h"
#include "debugger.h"
#include "filehelper.h"
#include <thread>

enum CMDRESULT
{
    STATUS_ERROR = false,
    STATUS_CONTINUE = true,
    STATUS_EXIT = 2,
    STATUS_PAUSE = 3,
    STATUS_CONTINUE_BRANCH = 4
};

static std::vector<LINEMAPENTRY> linemap;
static std::vector<SCRIPTBP> scriptbplist;
static std::vector<int> scriptstack;
static int scriptIp = 0;
static int scriptIpOld = 0;
static bool volatile bAbort = false;
static bool volatile bIsRunning = false;
static bool scriptLogEnabled = false;
static CMDRESULT scriptLastError = STATUS_ERROR;

static SCRIPTBRANCHTYPE scriptgetbranchtype(const char* text)
{
    char newtext[MAX_SCRIPT_LINE_SIZE] = "";
    strcpy_s(newtext, StringUtils::Trim(text).c_str());
    if(!strstr(newtext, " "))
        strcat_s(newtext, " ");
    if(!strncmp(newtext, "jmp ", 4) || !strncmp(newtext, "goto ", 5))
        return scriptjmp;
    else if(!strncmp(newtext, "jbe ", 4) || !strncmp(newtext, "ifbe ", 5) || !strncmp(newtext, "ifbeq ", 6) || !strncmp(newtext, "jle ", 4) || !strncmp(newtext, "ifle ", 5) || !strncmp(newtext, "ifleq ", 6))
        return scriptjbejle;
    else if(!strncmp(newtext, "jae ", 4) || !strncmp(newtext, "ifae ", 5) || !strncmp(newtext, "ifaeq ", 6) || !strncmp(newtext, "jge ", 4) || !strncmp(newtext, "ifge ", 5) || !strncmp(newtext, "ifgeq ", 6))
        return scriptjaejge;
    else if(!strncmp(newtext, "jne ", 4) || !strncmp(newtext, "ifne ", 5) || !strncmp(newtext, "ifneq ", 6) || !strncmp(newtext, "jnz ", 4) || !strncmp(newtext, "ifnz ", 5))
        return scriptjnejnz;
    else if(!strncmp(newtext, "je ", 3)  || !strncmp(newtext, "ife ", 4) || !strncmp(newtext, "ifeq ", 5) || !strncmp(newtext, "jz ", 3) || !strncmp(newtext, "ifz ", 4))
        return scriptjejz;
    else if(!strncmp(newtext, "jb ", 3) || !strncmp(newtext, "ifb ", 4) || !strncmp(newtext, "jl ", 3) || !strncmp(newtext, "ifl ", 4))
        return scriptjbjl;
    else if(!strncmp(newtext, "ja ", 3) || !strncmp(newtext, "ifa ", 4) || !strncmp(newtext, "jg ", 3) || !strncmp(newtext, "ifg ", 4))
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

static inline bool isEmptyLine(SCRIPTLINETYPE type)
{
    return type == lineempty || type == linecomment || type == linelabel;
}

static int scriptinternalstep(int fromIp) //internal step routine
{
    int maxIp = (int)linemap.size(); //maximum ip
    if(fromIp >= maxIp) //script end
        return fromIp;
    while(isEmptyLine(linemap.at(fromIp).type) && fromIp < maxIp) //skip empty lines
        fromIp++;
    fromIp++;
    return fromIp;
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

static bool scriptcreatelinemap(const char* filename)
{
    String filedata;
    if(!FileHelper::ReadAllText(filename, filedata))
    {
        String TranslatedString = GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "FileHelper::ReadAllText failed..."));
        GuiScriptError(0, TranslatedString.c_str());
        return false;
    }
    auto len = filedata.length();
    char temp[256] = "";
    LINEMAPENTRY entry;
    memset(&entry, 0, sizeof(entry));
    linemap.clear();
    for(size_t i = 0, j = 0; i < len; i++) //make raw line map
    {
        if(filedata[i] == '\r' && filedata[i + 1] == '\n') //windows file
        {
            memset(&entry, 0, sizeof(entry));
            int add = 0;
            while(isspace(temp[add]))
                add++;
            strcpy_s(entry.raw, temp + add);
            *temp = 0;
            j = 0;
            i++;
            linemap.push_back(entry);
        }
        else if(filedata[i] == '\n') //other file
        {
            memset(&entry, 0, sizeof(entry));
            int add = 0;
            while(isspace(temp[add]))
                add++;
            strcpy_s(entry.raw, temp + add);
            *temp = 0;
            j = 0;
            linemap.push_back(entry);
        }
        else if(j >= 254)
        {
            memset(&entry, 0, sizeof(entry));
            int add = 0;
            while(isspace(temp[add]))
                add++;
            strcpy_s(entry.raw, temp + add);
            *temp = 0;
            j = 0;
            linemap.push_back(entry);
        }
        else
            j += sprintf_s(temp + j, sizeof(temp) - j, "%c", filedata[i]);
    }
    if(*temp)
    {
        memset(&entry, 0, sizeof(entry));
        strcpy_s(entry.raw, temp);
        linemap.push_back(entry);
    }
    int linemapsize = (int)linemap.size();
    while(linemapsize && !*linemap.at(linemapsize - 1).raw) //remove empty lines from the end
    {
        linemapsize--;
        linemap.pop_back();
    }
    for(int i = 0; i < linemapsize; i++)
    {
        LINEMAPENTRY cur = linemap.at(i);

        //temp. remove comments from the raw line
        char line_comment[256] = "";
        char* comment = nullptr;
        {
            auto len = strlen(cur.raw);
            auto inquote = false;
            auto inescape = false;
            for(size_t i = 0; i < len; i++)
            {
                auto ch = cur.raw[i];
                switch(ch) //simple state machine to determine if the "//" is in quotes
                {
                case '\"':
                    if(!inescape)
                        inquote = !inquote;
                    inescape = false;
                    break;
                case '\\':
                    inescape = !inescape;
                    break;
                default:
                    inescape = false;
                }
                if(!inquote && ch == '/' && i + 1 < len && cur.raw[i + 1] == '/')
                {
                    comment = cur.raw + i;
                    break;
                }
            }
        }

        if(comment && comment != cur.raw) //only when the line doesnt start with a comment
        {
            if(*(comment - 1) == ' ') //space before comment
            {
                strcpy_s(line_comment, comment);
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
        else if(!strncmp(cur.raw, "//", 2) || *cur.raw == ';') //comment
        {
            cur.type = linecomment;
            strcpy_s(cur.u.comment, cur.raw);
        }
        else if(cur.raw[rawlen - 1] == ':') //label
        {
            cur.type = linelabel;
            sprintf_s(cur.u.label, "l %.*s", rawlen - 1, cur.raw); //create a fake command for formatting
            strcpy_s(cur.u.label, StringUtils::Trim(cur.u.label).c_str());
            strcpy_s(temp, cur.u.label + 2);
            strcpy_s(cur.u.label, temp); //remove fake command
            if(!*cur.u.label || !strcmp(cur.u.label, "\"\"")) //no label text
            {
                char message[256] = "";
                sprintf_s(message, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Empty label detected on line %d!")), i + 1);
                GuiScriptError(0, message);
                linemap.clear();
                return false;
            }
            int foundlabel = scriptlabelfind(cur.u.label);
            if(foundlabel) //label defined twice
            {
                char message[256] = "";
                sprintf_s(message, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Duplicate label \"%s\" detected on lines %d and %d!")), cur.u.label, foundlabel, i + 1);
                GuiScriptError(0, message);
                linemap.clear();
                return false;
            }
        }
        else if(scriptgetbranchtype(cur.raw) != scriptnobranch) //branch
        {
            cur.type = linebranch;
            cur.u.branch.type = scriptgetbranchtype(cur.raw);
            char newraw[MAX_SCRIPT_LINE_SIZE] = "";
            strcpy_s(newraw, StringUtils::Trim(cur.raw).c_str());
            int rlen = (int)strlen(newraw);
            for(int j = 0; j < rlen; j++)
                if(newraw[j] == ' ')
                {
                    strcpy_s(cur.u.branch.branchlabel, newraw + j + 1);
                    break;
                }
        }
        else
        {
            cur.type = linecommand;
            strcpy_s(cur.u.command, cur.raw);
        }

        //append the comment to the raw line again
        if(*line_comment)
            sprintf_s(cur.raw + rawlen, sizeof(cur.raw) - rawlen, "\1%s", line_comment);
        linemap.at(i) = cur;
    }
    linemapsize = (int)linemap.size();
    for(int i = 0; i < linemapsize; i++)
    {
        auto & currentLine = linemap.at(i);
        if(currentLine.type == linebranch) //invalid branch label
        {
            int labelline = scriptlabelfind(currentLine.u.branch.branchlabel);
            if(!labelline) //invalid branch label
            {
                char message[256] = "";
                sprintf_s(message, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Invalid branch label \"%s\" detected on line %d!")), currentLine.u.branch.branchlabel, i + 1);
                GuiScriptError(0, message);
                linemap.clear();
                return false;
            }
            else //set the branch destination line
                currentLine.u.branch.dest = scriptinternalstep(labelline);
        }
    }
    // append a ret instruction at the end of the script when appropriate
    if(!linemap.empty())
    {
        memset(&entry, 0, sizeof(entry));
        entry.type = linecommand;
        strcpy_s(entry.raw, "ret");
        strcpy_s(entry.u.command, "ret");

        const auto & lastline = linemap.back();
        switch(lastline.type)
        {
        case linecommand:
            if(scriptisinternalcommand(lastline.u.command, "ret")
                    || scriptisinternalcommand(lastline.u.command, "invalid")
                    || scriptisinternalcommand(lastline.u.command, "error"))
            {
                // there is already a terminating command at the end of the script
            }
            else
            {
                linemap.push_back(entry);
            }
            break;
        case linebranch:
            // an unconditional branch at the end of the script can only go back
            if(lastline.u.branch.type == scriptjmp)
                break;
        // fallthough to append the ret
        case linelabel:
        case linecomment:
        case lineempty:
            linemap.push_back(entry);
            break;
        }
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
    if(!line || line > (int)linemap.size()) //invalid line
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

static bool scriptinternalbranch(SCRIPTBRANCHTYPE type) //determine if we should jump
{
    duint ezflag = 0;
    duint bsflag = 0;
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
        if(!bsflag && !ezflag)
            bJump = true;
        break;
    case scriptjajg: //$_BS_FLAG=1 and $_EZ_FLAG=0 //above, not equal
        if(bsflag && !ezflag)
            bJump = true;
        break;
    case scriptjbejle: //$_BS_FLAG=0 or $_EZ_FLAG=1
        if(!bsflag || ezflag)
            bJump = true;
        break;
    case scriptjaejge: //$_BS_FLAG=1 or $_EZ_FLAG=1
        if(bsflag || ezflag)
            bJump = true;
        break;
    default:
        bJump = false;
        break;
    }
    return bJump;
}

static CMDRESULT scriptinternalcmdexec(const char* cmd, bool silentRet)
{
    if(scriptisinternalcommand(cmd, "ret")) //script finished
    {
        if(!scriptstack.size()) //nothing on the stack
        {
            if(!silentRet)
            {
                String TranslatedString = GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Script finished!"));
                GuiScriptMessage(TranslatedString.c_str());
            }
            return STATUS_EXIT;
        }
        scriptIp = scriptstack.back();
        scriptstack.pop_back(); //remove last stack entry
        return STATUS_CONTINUE_BRANCH;
    }
    else if(scriptisinternalcommand(cmd, "error")) //show an error and end the script
    {
        GuiScriptError(0, StringUtils::Trim(cmd + strlen("error"), " \"'").c_str());
        return STATUS_EXIT;
    }
    else if(scriptisinternalcommand(cmd, "invalid")) //invalid command for testing
        return STATUS_ERROR;
    else if(scriptisinternalcommand(cmd, "pause")) //pause the script
        return STATUS_PAUSE;
    else if(scriptisinternalcommand(cmd, "nop")) //do nothing
        return STATUS_CONTINUE;
    else if(scriptisinternalcommand(cmd, "log"))
        scriptLogEnabled = true;
    //super disgusting hack(s) to support branches in the GUI
    {
        auto branchtype = scriptgetbranchtype(cmd);
        if(branchtype != scriptnobranch)
        {
            String cmdStr = cmd;
            auto branchlabel = StringUtils::Trim(cmdStr.substr(cmdStr.find(' ')));
            auto labelIp = scriptlabelfind(branchlabel.c_str());
            if(!labelIp)
            {
                char message[256] = "";
                sprintf_s(message, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Invalid branch label \"%s\" detected on line %d!")), branchlabel.c_str(), 0);
                GuiScriptError(0, message);
                return STATUS_ERROR;
            }
            if(scriptinternalbranch(branchtype))
            {
                if(branchtype == scriptcall) //calls have a special meaning
                    scriptstack.push_back(scriptIp);
                scriptIp = scriptinternalstep(labelIp); //go to the first command after the label
                GuiScriptSetIp(scriptIp);
            }
            return STATUS_CONTINUE_BRANCH;
        }
    }
    auto res = cmddirectexec(cmd);
    while(DbgIsDebugging() && dbgisrunning() && !bAbort) //while not locked (NOTE: possible deadlock)
    {
        Sleep(1);
        GuiProcessEvents(); //workaround for scripts being executed on the GUI thread
    }
    scriptLogEnabled = false;
    return res ? STATUS_CONTINUE : STATUS_ERROR;
}

static bool scriptinternalcmd(bool silentRet)
{
    bool bContinue = true;
    if(size_t(scriptIp - 1) >= linemap.size())
        return false;
    const LINEMAPENTRY & cur = linemap.at(scriptIp - 1);
    scriptIpOld = scriptIp;
    scriptIp = scriptinternalstep(scriptIp);
    if(cur.type == linecommand)
    {
        scriptLastError = scriptinternalcmdexec(cur.u.command, silentRet);
        switch(scriptLastError)
        {
        case STATUS_CONTINUE:
        case STATUS_CONTINUE_BRANCH:
            if(scriptIp == scriptIpOld)
            {
                bContinue = false;
                scriptIp = scriptinternalstep(0);
            }
            break;
        case STATUS_ERROR:
            bContinue = false;
            scriptIp = scriptIpOld;
            GuiScriptError(scriptIp, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Error executing command!")));
            break;
        case STATUS_EXIT:
            bContinue = false;
            scriptIp = scriptinternalstep(0);
            GuiScriptSetIp(scriptIp);
            break;
        case STATUS_PAUSE:
            bContinue = false; //stop running the script
            GuiScriptSetIp(scriptIp);
            break;
        }
    }
    else if(cur.type == linebranch)
    {
        if(scriptinternalbranch(cur.u.branch.type))
        {
            if(cur.u.branch.type == scriptcall) //calls have a special meaning
                scriptstack.push_back(scriptIp);
            scriptIp = scriptlabelfind(cur.u.branch.branchlabel);
            scriptIp = scriptinternalstep(scriptIp); //go to the first command after the label
        }
    }
    return bContinue;
}

static DWORD WINAPI scriptLoadSyncThread(LPVOID filename)
{
    scriptLoadSync(reinterpret_cast<const char*>(filename));
    return 0;
}

bool scriptRunSync(int destline, bool silentRet)
{
    // disable GUI updates
    auto guiUpdateWasDisabled = GuiIsUpdateDisabled();
    if(!guiUpdateWasDisabled)
        GuiUpdateDisable();
    if(!destline || destline > (int)linemap.size()) //invalid line
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
    bool bIgnoreTimeout = settingboolget("Engine", "NoScriptTimeout");
    unsigned long long kernelTime, userTime;
    FILETIME creationTime, exitTime; // unused
    while(bContinue && !bAbort) //run loop
    {
        bContinue = scriptinternalcmd(silentRet);
        if(scriptinternalbpget(scriptIp)) //breakpoint=stop run loop
            bContinue = false;
        if(bContinue && !bIgnoreTimeout && GetThreadTimes(GetCurrentThread(), &creationTime, &exitTime, reinterpret_cast<LPFILETIME>(&kernelTime), reinterpret_cast<LPFILETIME>(&userTime)) != 0)
        {
            if(userTime + kernelTime >= 10 * 10000000) // time out in 10 seconds of CPU time
            {
                if(GuiScriptMsgyn(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "The script is too busy. Would you like to terminate it now?"))) != 0)
                {
                    dputs(QT_TRANSLATE_NOOP("DBG", "Script is terminated by user."));
                    break;
                }
                else
                    bIgnoreTimeout = true;
            }
        }
    }
    bIsRunning = false; //not running anymore
    // re-enable GUI updates when appropriate
    if(!guiUpdateWasDisabled)
        GuiUpdateEnable(true);
    GuiScriptSetIp(scriptIp);
    // the script fully executed (which means scriptIp is reset to the first line), without any errors
    return scriptIp == scriptinternalstep(0) && (scriptLastError == STATUS_EXIT || scriptLastError == STATUS_CONTINUE || scriptLastError == STATUS_CONTINUE_BRANCH);
}

bool scriptLoadSync(const char* filename)
{
    GuiScriptClear();
    GuiScriptEnableHighlighting(true); //enable default script syntax highlighting
    scriptIp = 0;
    scriptIpOld = 0;
    scriptbplist.clear();
    scriptstack.clear();
    bAbort = false;
    if(!scriptcreatelinemap(reinterpret_cast<const char*>(filename)))
        return false; // Script load failed
    int lines = (int)linemap.size();
    const char** script = reinterpret_cast<const char**>(BridgeAlloc(lines * sizeof(const char*)));
    for(int i = 0; i < lines; i++) //add script lines
        script[i] = linemap.at(i).raw;
    GuiScriptAdd(lines, script);
    scriptIp = scriptinternalstep(0);
    GuiScriptSetIp(scriptIp);
    return true;
}

void scriptload(const char* filename)
{
    static char filename_[MAX_PATH] = "";
    strcpy_s(filename_, filename);
    auto hThread = CreateThread(nullptr, 0, scriptLoadSyncThread, filename_, 0, nullptr);
    while(WaitForSingleObject(hThread, 100) == WAIT_TIMEOUT)
        GuiProcessEvents();
    CloseHandle(hThread);
}

void scriptunload()
{
    GuiScriptClear();
    linemap.clear();
    scriptbplist.clear();
    scriptstack.clear();
    scriptIp = 0;
    scriptIpOld = 0;
    bAbort = false;
}

void scriptrun(int destline, bool silentRet)
{
    if(DbgIsDebugging() && dbgisrunning())
    {
        if(!silentRet)
            GuiScriptError(0, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Debugger must be paused to run a script!")));
        else
            dputs(QT_TRANSLATE_NOOP("DBG", "Debugger must be paused to run a script!"));
        return;
    }
    if(bIsRunning) //already running
        return;
    bIsRunning = true;
    std::thread t([destline, silentRet]
    {
        scriptRunSync(destline, silentRet);
    });
    t.detach();
}

void scriptstep()
{
    std::thread t([]
    {
        if(!bIsRunning) //only step when not running
        {
            scriptinternalcmd(false);
            GuiScriptSetIp(scriptIp);
        }
    });
    t.detach();
}

bool scriptbptoggle(int line)
{
    if(!line || line > (int)linemap.size()) //invalid line
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
    scriptIpOld = scriptIp;
    scriptLastError = scriptinternalcmdexec(command, false);
    switch(scriptLastError)
    {
    case STATUS_ERROR:
        return false;
    case STATUS_EXIT:
        scriptIp = scriptinternalstep(0);
        GuiScriptSetIp(scriptIp);
        break;
    case STATUS_PAUSE:
    case STATUS_CONTINUE:
        break;
    case STATUS_CONTINUE_BRANCH:
        if(!bIsRunning)
        {
            scriptrun(scriptIp, true);
        }
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
    if(!info || !line || line > (int)linemap.size()) //invalid line
        return false;
    if(linemap.at(line - 1).type != linebranch) //no branch
        return false;
    memcpy(info, &linemap.at(line - 1).u.branch, sizeof(SCRIPTBRANCH));
    return true;
}

void scriptlog(const char* msg)
{
    if(!scriptLogEnabled)
        return;
    GuiScriptSetInfoLine(scriptIpOld, msg);
}
