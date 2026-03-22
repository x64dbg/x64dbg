/**
 @file simplescript.cpp

 @brief Implements the simplescript class.
 */

#include "simplescript.h"
#include "console.h"
#include "variable.h"
#include "debugger.h"
#include "filehelper.h"
#include "jobqueue.h"
#include "threading.h"
#include <atomic>

static JobQueue scriptQueue;

enum CMDRESULT
{
    STATUS_ERROR = 0,
    STATUS_CONTINUE,
    STATUS_EXIT,
    STATUS_PAUSE,
    STATUS_CONTINUE_BRANCH,
};

struct SCRIPTBP
{
    int line = 0;
    bool silent = false; //do not show in GUI
};

struct LINEMAPENTRY
{
    SCRIPTLINETYPE type;
    char raw[256];
    union
    {
        char command[256];
        SCRIPTBRANCH branch;
        char label[256];
        char comment[256];
    } u;
};

struct SCRIPTFRAME
{
    int ip = 0;
    SCRIPTSTATE state = SCRIPT_PAUSED;
};

static std::vector<LINEMAPENTRY> scriptLineMap;
static std::vector<SCRIPTBP> scriptBpList;
static std::vector<SCRIPTFRAME> scriptStack;
static int scriptIp = 0;
static int scriptIpOld = 0;
static std::atomic<SCRIPTSTATE> scriptState;
static std::atomic<ScriptInterrupt> gScriptInterruptReason{ ScriptInterrupt::None };
enum class ScriptPostYieldAction
{
    None = 0,
    Pause,
    Abort,
};
static std::atomic<ScriptPostYieldAction> gScriptPostYieldAction{ ScriptPostYieldAction::None };
static std::atomic_bool gScriptYieldActive{ false };
static std::atomic_bool bScriptLogEnabled;
static CMDRESULT scriptLastError = STATUS_ERROR;
static bool bRunGui = false;
static bool gScriptCreatedDebugSession = false; // observed false->true transition during script execution

static HANDLE scriptYieldedEvent()
{
    static HANDLE event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    return event;
}

static HANDLE scriptResumeEvent()
{
    static HANDLE event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    return event;
}

static bool scriptIsAbortReason(ScriptInterrupt reason)
{
    switch(reason)
    {
    case ScriptInterrupt::AbortUser:
    case ScriptInterrupt::AbortAssertion:
    case ScriptInterrupt::AbortShutdown:
        return true;
    default:
        return false;
    }
}

static void scriptResetInterruptState()
{
    gScriptInterruptReason = ScriptInterrupt::None;
    gScriptPostYieldAction = ScriptPostYieldAction::None;
    gScriptYieldActive = false;
    ResetEvent(scriptYieldedEvent());
    ResetEvent(scriptResumeEvent());
}

static void scriptRequestPostYieldAction(ScriptPostYieldAction action)
{
    auto current = gScriptPostYieldAction.load();
    while(current < action && !gScriptPostYieldAction.compare_exchange_weak(current, action))
    {
    }
}

static void scriptUpdateDebugSessionOwnership(bool wasDebugging, bool isDebugging)
{
    if(!wasDebugging && isDebugging)
        gScriptCreatedDebugSession = true;
    else if(wasDebugging && !isDebugging && gScriptCreatedDebugSession)
        gScriptCreatedDebugSession = false;
}

static bool scriptHandleInterrupt()
{
    while(true)
    {
        auto reason = gScriptInterruptReason.load();
        if(reason == ScriptInterrupt::None)
            return false;
        if(reason == ScriptInterrupt::YieldDebugEvent)
        {
            gScriptYieldActive = true;
            SetEvent(scriptYieldedEvent());
            while(gScriptInterruptReason.load() == ScriptInterrupt::YieldDebugEvent)
                WaitForSingleObject(scriptResumeEvent(), 100);
            gScriptYieldActive = false;
            ResetEvent(scriptYieldedEvent());
            continue;
        }
        return scriptIsAbortReason(reason);
    }
}

static SCRIPTBRANCHTYPE scriptGetBranchType(const char* text)
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

static int scriptLabelFind(const char* labelname)
{
    SHARED_ACQUIRE(LockScriptLineMap);
    int linecount = (int)scriptLineMap.size();
    for(int i = 0; i < linecount; i++)
        if(scriptLineMap.at(i).type == linelabel && !strcmp(scriptLineMap.at(i).u.label, labelname))
            return i + 1;
    return 0;
}

static bool isEmptyLine(SCRIPTLINETYPE type)
{
    return type == lineempty || type == linecomment || type == linelabel;
}

static int scriptNextIp(int fromIp) //internal step routine
{
    SHARED_ACQUIRE(LockScriptLineMap);
    int maxIp = (int)scriptLineMap.size(); //maximum ip
    if(fromIp >= maxIp) //script end
        return fromIp;
    while(isEmptyLine(scriptLineMap.at(fromIp).type) && fromIp < maxIp) //skip empty lines
        fromIp++;
    fromIp++;
    return fromIp;
}

static bool scriptIsInternalCommand(const char* text, const char* cmd)
{
    int len = (int)strlen(text);
    int cmdlen = (int)strlen(cmd);
    if(cmdlen > len)
        return false;
    if(cmdlen == len)
        return scmp(text, cmd);
    if(text[cmdlen] == ' ')
        return _strnicmp(text, cmd, cmdlen) == 0;
    return false;
}

static void scriptError(int line, const char* error, bool gui)
{
    if(gui)
        GuiScriptError(line, error);
    else
        dputs_untranslated(error);
}

static bool scriptCreateLineMap(const char* filename, bool gui)
{
    String filedata;
    if(!FileHelper::ReadAllText(filename, filedata))
    {
        scriptError(0, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "FileHelper::ReadAllText failed...")), gui);
        return false;
    }
    auto len = filedata.length();
    char temp[256] = "";
    LINEMAPENTRY entry = {};
    scriptLineMap.clear();
    for(size_t i = 0, j = 0; i < len; i++) //make raw line map
    {
        if(filedata[i] == '\r' && filedata[i + 1] == '\n') //windows file
        {
            entry = {};
            int add = 0;
            while(isspace(temp[add]))
                add++;
            strcpy_s(entry.raw, temp + add);
            *temp = 0;
            j = 0;
            i++;
            scriptLineMap.push_back(entry);
        }
        else if(filedata[i] == '\n') //other file
        {
            entry = {};
            int add = 0;
            while(isspace(temp[add]))
                add++;
            strcpy_s(entry.raw, temp + add);
            *temp = 0;
            j = 0;
            scriptLineMap.push_back(entry);
        }
        else if(j >= 254)
        {
            entry = {};
            int add = 0;
            while(isspace(temp[add]))
                add++;
            strcpy_s(entry.raw, temp + add);
            *temp = 0;
            j = 0;
            scriptLineMap.push_back(entry);
        }
        else
            j += sprintf_s(temp + j, sizeof(temp) - j, "%c", filedata[i]);
    }
    if(*temp)
    {
        entry = {};
        strcpy_s(entry.raw, temp);
        scriptLineMap.push_back(entry);
    }
    int linemapsize = (int)scriptLineMap.size();
    while(linemapsize && !*scriptLineMap.at(linemapsize - 1).raw) //remove empty lines from the end
    {
        linemapsize--;
        scriptLineMap.pop_back();
    }
    for(int i = 0; i < linemapsize; i++)
    {
        LINEMAPENTRY cur = scriptLineMap.at(i);

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

        if(comment && comment != cur.raw) //only when the line doesn't start with a comment
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
        while((rawlen > 0) && isspace(cur.raw[rawlen - 1]))  //Trim trailing whitespace
        {
            rawlen--;
        }
        cur.raw[rawlen] = '\0'; //// Truncate the string by setting a new null terminator
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
                scriptError(0, message, gui);
                scriptLineMap.clear();
                return false;
            }
            int foundlabel = scriptLabelFind(cur.u.label);
            if(foundlabel) //label defined twice
            {
                char message[256] = "";
                sprintf_s(message, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Duplicate label \"%s\" detected on lines %d and %d!")), cur.u.label, foundlabel, i + 1);
                scriptError(0, message, gui);
                scriptLineMap.clear();
                return false;
            }
        }
        else if(scriptGetBranchType(cur.raw) != scriptnobranch) //branch
        {
            cur.type = linebranch;
            cur.u.branch.type = scriptGetBranchType(cur.raw);
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
        scriptLineMap.at(i) = cur;
    }
    linemapsize = (int)scriptLineMap.size();
    for(int i = 0; i < linemapsize; i++)
    {
        auto & currentLine = scriptLineMap.at(i);
        if(currentLine.type == linebranch) //invalid branch label
        {
            int labelline = scriptLabelFind(currentLine.u.branch.branchlabel);
            if(!labelline) //invalid branch label
            {
                char message[256] = "";
                sprintf_s(message, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Invalid branch label \"%s\" detected on line %d!")), currentLine.u.branch.branchlabel, i + 1);
                scriptError(0, message, gui);
                scriptLineMap.clear();
                return false;
            }
            else //set the branch destination line
                currentLine.u.branch.dest = scriptNextIp(labelline);
        }
    }
    // append a ret instruction at the end of the script when appropriate
    if(!scriptLineMap.empty())
    {
        entry = {};
        entry.type = linecommand;
        strcpy_s(entry.raw, "ret");
        strcpy_s(entry.u.command, "ret");

        const auto & lastline = scriptLineMap.back();
        switch(lastline.type)
        {
        case linecommand:
            if(scriptIsInternalCommand(lastline.u.command, "ret")
                    || scriptIsInternalCommand(lastline.u.command, "invalid")
                    || scriptIsInternalCommand(lastline.u.command, "error"))
            {
                // there is already a terminating command at the end of the script
            }
            else
            {
                scriptLineMap.push_back(entry);
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
            scriptLineMap.push_back(entry);
            break;
        }
    }
    return true;
}

static bool scriptInternalBpGet(int line) //internal bpget routine
{
    SHARED_ACQUIRE(LockScriptBreakpoints);
    int bpcount = (int)scriptBpList.size();
    for(int i = 0; i < bpcount; i++)
        if(scriptBpList.at(i).line == line)
            return true;
    return false;
}

static bool scriptInternalBpToggle(int line) //internal breakpoint
{
    SHARED_ACQUIRE(LockScriptLineMap);
    EXCLUSIVE_ACQUIRE(LockScriptBreakpoints);

    if(!line || line > (int)scriptLineMap.size()) //invalid line
        return false;
    line = scriptNextIp(line - 1); //no breakpoints on non-executable locations

    if(scriptInternalBpGet(line)) //remove breakpoint
    {
        int bpcount = (int)scriptBpList.size();
        for(int i = 0; i < bpcount; i++)
            if(scriptBpList.at(i).line == line)
            {
                scriptBpList.erase(scriptBpList.begin() + i);
                break;
            }
    }
    else //add breakpoint
    {
        SCRIPTBP newbp = {};
        newbp.silent = true;
        newbp.line = line;
        scriptBpList.push_back(newbp);
    }
    return true;
}

static bool scriptBranchTaken(SCRIPTBRANCHTYPE type) //determine if we should jump
{
    duint ezflag = 0;
    duint bsflag = 0;
    varget("$_EZ_FLAG", &ezflag, nullptr, nullptr);
    varget("$_BS_FLAG", &bsflag, nullptr, nullptr);
    switch(type)
    {
    case scriptcall:
    case scriptjmp:
        return true;
    case scriptjnejnz: //$_EZ_FLAG=0
        return !ezflag;
    case scriptjejz: //$_EZ_FLAG=1
        return ezflag;
    case scriptjbjl: //$_BS_FLAG=0 and $_EZ_FLAG=0 //below, not equal
        return !bsflag && !ezflag;
    case scriptjajg: //$_BS_FLAG=1 and $_EZ_FLAG=0 //above, not equal
        return bsflag && !ezflag;
    case scriptjbejle: //$_BS_FLAG=0 or $_EZ_FLAG=1
        return !bsflag || ezflag;
    case scriptjaejge: //$_BS_FLAG=1 or $_EZ_FLAG=1
        return bsflag || ezflag;
    default:
        return false;
    }
}

static CMDRESULT scriptInternalCmdExec(const char* cmd, bool gui, SCRIPTSTATE state)
{
    if(scriptIsInternalCommand(cmd, "ret")) //script finished
    {
        if(scriptStack.empty()) //nothing on the stack
        {
            scriptIp = scriptNextIp(0);
            GuiScriptSetIp(scriptIp);

            auto message = GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Script finished!"));
            if(gui)
                GuiScriptMessage(message);
            else
                dputs_untranslated(message);
            return STATUS_EXIT;
        }
        auto frame = scriptStack.back();
        scriptIp = frame.ip;
        scriptStack.pop_back();

        // Pause if we were stepping, regardless of the frame state
        // Scenario:
        // - Run a script
        // - Set a script breakpoint inside 'mylabel'
        // - Hit a breakpoint triggers 'scriptcmd call mylabel'
        // - Script breakpoint hits
        // - Step over the 'ret'
        if(state == SCRIPT_STEPPING)
            return STATUS_PAUSE;

        // Pause if we were not running when the frame was pushed
        // Scenario:
        // - Step over an 'erun'
        // - Hit breakpoint triggers 'scriptcmd call'
        // - You will pause at the label
        // - Resume script running
        // - You will pause again after the 'erun'
        return frame.state != SCRIPT_RUNNING ? STATUS_PAUSE : STATUS_CONTINUE_BRANCH;
    }
    if(scriptIsInternalCommand(cmd, "error")) //show an error and end the script
    {
        scriptIp = scriptNextIp(0);
        GuiScriptSetIp(scriptIp);

        auto message = StringUtils::Trim(cmd + strlen("error"), " \"'");
        scriptError(scriptIpOld, message.c_str(), gui);
        return STATUS_EXIT;
    }
    if(scriptIsInternalCommand(cmd, "invalid")) //invalid command for testing
        return STATUS_ERROR;
    if(state != SCRIPT_PAUSED && (scriptIsInternalCommand(cmd, "scriptrun") || scriptIsInternalCommand(cmd, "scriptexec")))  // do not allow recursive script runs
        return STATUS_ERROR;
    if(scriptIsInternalCommand(cmd, "pause")) //pause the script
        return STATUS_PAUSE;
    if(scriptIsInternalCommand(cmd, "nop")) //do nothing
        return state == SCRIPT_RUNNING ? STATUS_CONTINUE : STATUS_PAUSE;
    if(scriptIsInternalCommand(cmd, "log"))
        bScriptLogEnabled = true;
    //super disgusting hack(s) to support branches in the GUI
    {
        auto branchtype = scriptGetBranchType(cmd);
        if(branchtype != scriptnobranch)
        {
            String cmdStr = cmd;
            auto branchlabel = StringUtils::Trim(cmdStr.substr(cmdStr.find(' ')));
            auto labelIp = scriptLabelFind(branchlabel.c_str());
            if(!labelIp)
            {
                char message[256] = "";
                sprintf_s(message, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Invalid branch label \"%s\" detected on line %d!")), branchlabel.c_str(), 0);
                scriptError(0, message, gui);
                return STATUS_ERROR;
            }
            if(scriptBranchTaken(branchtype))
            {
                if(branchtype == scriptcall) //calls have a special meaning
                    scriptStack.push_back({scriptIp, state});
                scriptIp = scriptNextIp(labelIp); //go to the first command after the label
                GuiScriptSetIp(scriptIp);
            }
            return state == SCRIPT_STEPPING ? STATUS_PAUSE : STATUS_CONTINUE_BRANCH;
        }
    }
    auto debuggingBefore = DbgIsDebugging();
    auto res = cmddirectexec(cmd);
    // Wait for the debuggee to pause
    while(bIsDebugging && dbgisrunning())
    {
        if(scriptHandleInterrupt())
        {
            bScriptLogEnabled = false;
            return STATUS_ERROR;
        }
        Sleep(1);
    }
    auto debuggingAfter = DbgIsDebugging();
    scriptUpdateDebugSessionOwnership(debuggingBefore, debuggingAfter);
    bScriptLogEnabled = false;
    auto postYieldAction = gScriptPostYieldAction.exchange(ScriptPostYieldAction::None);
    if(scriptHandleInterrupt() || postYieldAction == ScriptPostYieldAction::Abort || !res)
        return STATUS_ERROR;
    if(postYieldAction == ScriptPostYieldAction::Pause)
        return STATUS_PAUSE;
    return state == SCRIPT_RUNNING ? STATUS_CONTINUE : STATUS_PAUSE;
}

static bool scriptInternalCmd(bool gui, SCRIPTSTATE state)
{
    SHARED_ACQUIRE(LockScriptLineMap);
    bool bContinue = true;
    if(size_t(scriptIp - 1) >= scriptLineMap.size())
        return false;
    const LINEMAPENTRY & cur = scriptLineMap.at(scriptIp - 1);
    scriptIpOld = scriptIp;
    scriptIp = scriptNextIp(scriptIp);
    if(cur.type == linecommand)
    {
        scriptLastError = scriptInternalCmdExec(cur.u.command, gui, state);
        switch(scriptLastError)
        {
        case STATUS_CONTINUE:
        case STATUS_CONTINUE_BRANCH:
            break;
        case STATUS_ERROR:
            scriptIp = scriptIpOld;
            scriptError(scriptIp, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Error executing command!")), gui);
            return false;
        case STATUS_EXIT:
            return false;
        case STATUS_PAUSE:
            GuiScriptSetIp(scriptIp);
            return false;
        }
    }
    else if(cur.type == linebranch)
    {
        if(scriptBranchTaken(cur.u.branch.type))
        {
            if(cur.u.branch.type == scriptcall) //calls have a special meaning
                scriptStack.push_back({scriptIp, state});
            scriptIp = scriptLabelFind(cur.u.branch.branchlabel);
            scriptIp = scriptNextIp(scriptIp); //go to the first command after the label
        }
    }
    return true;
}

static bool scriptRun(int destline, bool gui)
{
    if(bIsDebugging && dbgisrunning())
    {
        scriptError(0, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Debugger must be paused to run a script!")), gui);
        return false;
    }

    // Use atomic compare_exchange to atomically check and set running state
    SCRIPTSTATE expectedState = SCRIPT_PAUSED;
    if(!scriptState.compare_exchange_strong(expectedState, SCRIPT_RUNNING))
        return false;

    bRunGui = gui;

    // disable GUI updates
    auto disabledGuiUpdates = !GuiIsUpdateDisabled();
    if(disabledGuiUpdates)
        GuiUpdateDisable();
    SHARED_ACQUIRE(LockScriptLineMap);
    if(destline == 0 || destline > (int)scriptLineMap.size()) //invalid line
        destline = 0;
    if(destline)
    {
        destline = scriptNextIp(destline - 1); //no breakpoints on non-executable locations
        if(!scriptInternalBpGet(destline)) //no breakpoint set
            scriptInternalBpToggle(destline);
    }
    scriptResetInterruptState();
    if(scriptIp)
        scriptIp--;
    scriptIp = scriptNextIp(scriptIp);
    bool bContinue = true;
    bool bIgnoreTimeout = settingboolget("Engine", "NoScriptTimeout", false);
    auto start = GetTickCount();
    while(bContinue) //run loop
    {
        if(scriptHandleInterrupt())
            break;
        bContinue = scriptInternalCmd(gui, SCRIPT_RUNNING);
        if(scriptInternalBpGet(scriptIp)) //breakpoint=stop run loop
            bContinue = false;
        if(bContinue && !bIgnoreTimeout)
        {
            if(GetTickCount() - start >= 30000) // time out in 30 seconds
            {
                if(GuiScriptMsgyn(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "The script has been running for a while. Would you like to terminate it now?"))) != 0)
                {
                    dputs(QT_TRANSLATE_NOOP("DBG", "Script is terminated by user."));
                    break;
                }
                start = GetTickCount();
            }
        }
    }
    auto interruptReason = gScriptInterruptReason.load();
    scriptState = SCRIPT_PAUSED; // set the script state to paused
    // re-enable GUI updates when appropriate
    if(disabledGuiUpdates)
        GuiUpdateEnable(true);
    GuiScriptSetIp(scriptIp);
    scriptResetInterruptState();
    // the script fully executed (which means scriptIp is reset to the first line), without any errors
    return !scriptIsAbortReason(interruptReason)
        && scriptIp == scriptNextIp(0)
        && (scriptLastError == STATUS_EXIT || scriptLastError == STATUS_CONTINUE || scriptLastError == STATUS_CONTINUE_BRANCH);
}

static bool scriptLoad(const char* filename, bool gui)
{
    ExclusiveSectionLocker<LockScriptLineMap> lockLineMap;
    ExclusiveSectionLocker<LockScriptBreakpoints> lockBreakpoints;
    GuiScriptClear();
    GuiScriptEnableHighlighting(true); //enable default script syntax highlighting
    scriptIp = 0;
    scriptIpOld = 0;
    scriptBpList.clear();
    scriptStack.clear();
    gScriptCreatedDebugSession = false;
    scriptResetInterruptState();
    if(!scriptCreateLineMap(filename, gui))
        return false; // Script load failed
    int lines = (int)scriptLineMap.size();
    auto script = (const char**)BridgeAlloc(lines * sizeof(const char*));
    for(int i = 0; i < lines; i++) //add script lines
        script[i] = scriptLineMap.at(i).raw;
    GuiScriptAdd(lines, script);
    scriptIp = scriptNextIp(0);
    GuiScriptSetIp(scriptIp);
    return true;
}

bool ScriptLoadAwait(const char* filename, bool gui)
{
    return scriptQueue.await([filename, gui]()
    {
        return scriptLoad(filename, gui);
    });
}

void ScriptUnloadAwait()
{
    scriptQueue.await([]()
    {
        ExclusiveSectionLocker<LockScriptLineMap> lockLineMap;
        ExclusiveSectionLocker<LockScriptBreakpoints> lockBreakpoints;
        GuiScriptClear();
        scriptLineMap.clear();
        scriptBpList.clear();
        scriptStack.clear();
        scriptIp = 0;
        scriptIpOld = 0;
        gScriptCreatedDebugSession = false;
        scriptResetInterruptState();
    });
}

void ScriptRunAsync(int destline, bool gui)
{
    scriptQueue.async([destline, gui]()
    {
        scriptRun(destline, gui);
    });
}

bool ScriptRunAwait(int destline, bool gui)
{
    return scriptQueue.await([destline, gui]()
    {
        return scriptRun(destline, gui);
    });
}

void ScriptStepAsync(bool gui)
{
    scriptQueue.async([gui]()
    {
        if(scriptState == SCRIPT_PAUSED)  // only step when the script is paused
        {
            scriptResetInterruptState();
            scriptState = SCRIPT_STEPPING;
            bRunGui = gui;
            scriptInternalCmd(gui, SCRIPT_STEPPING);
            scriptState = SCRIPT_PAUSED;
            scriptResetInterruptState();
            GuiScriptSetIp(scriptIp);
        }
    });
}

bool ScriptBpGetLocked(int line)
{
    SHARED_ACQUIRE(LockScriptBreakpoints);
    int bpcount = (int)scriptBpList.size();
    for(int i = 0; i < bpcount; i++)
        if(scriptBpList.at(i).line == line && !scriptBpList.at(i).silent)
            return true;
    return false;
}

bool ScriptBpToggleLocked(int line)
{
    SHARED_ACQUIRE(LockScriptLineMap);
    EXCLUSIVE_ACQUIRE(LockScriptBreakpoints);
    if(!line || line > (int)scriptLineMap.size()) //invalid line
        return false;
    auto bpline = scriptNextIp(line - 1); //no breakpoints on non-executable locations
    if(ScriptBpGetLocked(bpline)) //remove breakpoint
    {
        int bpcount = (int)scriptBpList.size();
        for(int i = 0; i < bpcount; i++)
            if(scriptBpList.at(i).line == bpline && !scriptBpList.at(i).silent)
            {
                scriptBpList.erase(scriptBpList.begin() + i);
                break;
            }
    }
    else //add breakpoint
    {
        SCRIPTBP newbp = {};
        newbp.silent = false;
        newbp.line = bpline;
        scriptBpList.push_back(newbp);
    }
    return true;
}

static bool scriptExecCommand(const char* command, bool gui, SCRIPTSTATE state)
{
    scriptIpOld = scriptIp;
    scriptLastError = scriptInternalCmdExec(command, gui, state);
    switch(scriptLastError)
    {
    case STATUS_ERROR:
        if(gScriptYieldActive.load())
            scriptRequestPostYieldAction(ScriptPostYieldAction::Abort);
        return false;
    case STATUS_EXIT:
    case STATUS_CONTINUE:
        break;
    case STATUS_PAUSE:
        if(gScriptYieldActive.load())
            scriptRequestPostYieldAction(ScriptPostYieldAction::Pause);
        break;
    case STATUS_CONTINUE_BRANCH:
        // If the user executes `scriptcmd call xxx`, start running.
        if(state == SCRIPT_PAUSED)
            ScriptRunAsync(scriptIp, gui);
        break;
    }
    return true;
}

bool ScriptCmdExecAwait(const char* command, bool gui, const SCRIPTSTATE* interruptState)
{
    // Capture the current script now, because this function might be called
    // through a breakpoint command or dbgcmdexecdirect("scriptcmd") from a plugin callback.
    // If we did not capture the script state it will be reset by the queue.
    // NOTE: Relevant if you step over 'erun' and a breakpoint with 'scriptcmd' is hit.
    auto state = interruptState != nullptr ? *interruptState : scriptState.load();
    if(gScriptYieldActive.load())
        return scriptExecCommand(command, gui, state);
    return scriptQueue.await([command, state, gui]()
    {
        return scriptExecCommand(command, gui, state);
    });
}

void ScriptSetIpAwait(int line)
{
    scriptQueue.await([line]()
    {
        if(line)
            scriptIp = scriptNextIp(line - 1);
        else
            scriptIp = scriptNextIp(0);
        GuiScriptSetIp(scriptIp);
    });
}

ScriptInterruptState ScriptInterruptAwait(ScriptInterrupt reason)
{
    auto state = scriptState.load();
    if(state == SCRIPT_PAUSED)
        return { state, bRunGui };

    if(reason == ScriptInterrupt::YieldDebugEvent)
    {
        ResetEvent(scriptYieldedEvent());
        ResetEvent(scriptResumeEvent());
        gScriptInterruptReason = reason;
        while(WaitForSingleObject(scriptYieldedEvent(), 100) == WAIT_TIMEOUT)
            GuiProcessEvents();
    }
    else
    {
        gScriptInterruptReason = reason;
        SetEvent(scriptResumeEvent());
        scriptQueue.await([]()
        {
            // Empty lambda just to wait for queue completion
        });
    }
    return { state, bRunGui };
}

void ScriptResume()
{
    if(gScriptInterruptReason.load() != ScriptInterrupt::YieldDebugEvent)
        return;
    gScriptInterruptReason = ScriptInterrupt::None;
    SetEvent(scriptResumeEvent());
}

SCRIPTLINETYPE ScriptGetLineTypeLocked(int line)
{
    SHARED_ACQUIRE(LockScriptLineMap);
    if(line > (int)scriptLineMap.size())
        return lineempty;
    return scriptLineMap.at(line - 1).type;
}

bool ScriptGetBranchInfoLocked(int line, SCRIPTBRANCH* info)
{
    SHARED_ACQUIRE(LockScriptLineMap);
    if(!info || !line || line > (int)scriptLineMap.size()) //invalid line
        return false;
    if(scriptLineMap.at(line - 1).type != linebranch) //no branch
        return false;
    memcpy(info, &scriptLineMap.at(line - 1).u.branch, sizeof(SCRIPTBRANCH));
    return true;
}

void ScriptLogLocked(const char* msg)
{
    if(!bScriptLogEnabled)
        return;
    GuiScriptSetInfoLine(scriptIpOld, msg);
}

bool ScriptExecAwait(const char* filename, bool gui)
{
    return scriptQueue.await([filename, gui]()
    {
        if(!scriptLoad(filename, gui))
            return false;
        if(!scriptRun(0, gui))
            return false;
        ScriptUnloadAwait();
        return true;
    });
}
