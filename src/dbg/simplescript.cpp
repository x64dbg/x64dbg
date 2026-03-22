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
    String raw;
    // NOTE: one of these (former union)
    String command;
    SCRIPTBRANCH branch;
    String label;
    String comment;
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
static std::atomic_bool bScriptAbort;
static std::atomic_bool bScriptLogEnabled;
static CMDRESULT scriptLastError = STATUS_ERROR;
static bool bRunGui = false;

static SCRIPTBRANCHTYPE scriptGetBranchType(const String & text)
{
    String newtext = StringUtils::Trim(text);
    if(newtext.find(" ") == std::string::npos)
        newtext += " ";
    if(StringUtils::StartsWith(newtext, "jmp ") || StringUtils::StartsWith(newtext, "goto "))
        return scriptjmp;
    else if(StringUtils::StartsWith(newtext, "jbe ") || StringUtils::StartsWith(newtext, "ifbe ") || StringUtils::StartsWith(newtext, "ifbeq ") || StringUtils::StartsWith(newtext, "jle ") || StringUtils::StartsWith(newtext, "ifle ") || StringUtils::StartsWith(newtext, "ifleq "))
        return scriptjbejle;
    else if(StringUtils::StartsWith(newtext, "jae ") || StringUtils::StartsWith(newtext, "ifae ") || StringUtils::StartsWith(newtext, "ifaeq ") || StringUtils::StartsWith(newtext, "jge ") || StringUtils::StartsWith(newtext, "ifge ") || StringUtils::StartsWith(newtext, "ifgeq "))
        return scriptjaejge;
    else if(StringUtils::StartsWith(newtext, "jne ") || StringUtils::StartsWith(newtext, "ifne ") || StringUtils::StartsWith(newtext, "ifneq ") || StringUtils::StartsWith(newtext, "jnz ") || StringUtils::StartsWith(newtext, "ifnz "))
        return scriptjnejnz;
    else if(StringUtils::StartsWith(newtext, "je ")  || StringUtils::StartsWith(newtext, "ife ") || StringUtils::StartsWith(newtext, "ifeq ") || StringUtils::StartsWith(newtext, "jz ") || StringUtils::StartsWith(newtext, "ifz "))
        return scriptjejz;
    else if(StringUtils::StartsWith(newtext, "jb ") || StringUtils::StartsWith(newtext, "ifb ") || StringUtils::StartsWith(newtext, "jl ") || StringUtils::StartsWith(newtext, "ifl "))
        return scriptjbjl;
    else if(StringUtils::StartsWith(newtext, "ja ") || StringUtils::StartsWith(newtext, "ifa ") || StringUtils::StartsWith(newtext, "jg ") || StringUtils::StartsWith(newtext, "ifg "))
        return scriptjajg;
    else if(StringUtils::StartsWith(newtext, "call "))
        return scriptcall;
    return scriptnobranch;
}

static int scriptLabelFind(const String & labelname)
{
    SHARED_ACQUIRE(LockScriptLineMap);
    int linecount = (int)scriptLineMap.size();
    for(int i = 0; i < linecount; i++)
        if(scriptLineMap.at(i).type == linelabel && (scriptLineMap.at(i).label == labelname))
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
    while((fromIp < maxIp) && isEmptyLine(scriptLineMap.at(fromIp).type)) //skip empty lines
        fromIp++;
    fromIp++;
    return fromIp;
}

static bool scriptIsInternalCommand(const String & text, const String & cmd)
{
    if(cmd.length() > text.length())
        return false;
    return StringUtils::CaseInsensitiveEqual()(text.substr(0, cmd.length()), cmd);
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
    String temp;
    LINEMAPENTRY entry = {};
    scriptLineMap.clear();
    for(size_t i = 0; i < len; i++) //make raw line map
    {
        if(filedata[i] == '\r' && ((i + 1) < len) && filedata[i + 1] == '\n') //windows file
        {
            entry = {};
            entry.raw = StringUtils::Trim(temp);
            temp = "";
            i++;
            scriptLineMap.push_back(entry);
        }
        else if(filedata[i] == '\n') //other file
        {
            entry = {};
            entry.raw = StringUtils::Trim(temp);
            temp = "";
            scriptLineMap.push_back(entry);
        }
        else
        {
            temp += filedata[i];
        }
    }

    temp = StringUtils::Trim(temp);

    if(temp.length())
    {
        entry = {};
        entry.raw = temp;
        scriptLineMap.push_back(entry);
    }

    int linemapsize = (int)scriptLineMap.size();
    while(linemapsize && scriptLineMap.at(linemapsize - 1).raw.length() == 0) //remove empty lines from the end
    {
        linemapsize--;
        scriptLineMap.pop_back();
    }

    for(int i = 0; i < linemapsize; i++)
    {
        LINEMAPENTRY cur = scriptLineMap.at(i);

        //temp. remove comments from the raw line
        String line_comment;
        String comment;
        {
            auto len = cur.raw.length();
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
                    comment = cur.raw.substr(i);
                    break;
                }
            }
        }

        if(comment.length() && comment != cur.raw) //only when the line doesn't start with a comment
        {
            line_comment = comment;
            //Remove the comment from the raw string
            cur.raw = StringUtils::Trim(cur.raw.substr(0, cur.raw.length() - line_comment.length()));
        }

        if(cur.raw.length() == 0) //empty
        {
            cur.type = lineempty;
        }
        else if(StringUtils::StartsWith(cur.raw, "//") || StringUtils::StartsWith(cur.raw, ";")) //comment
        {
            cur.type = linecomment;
            cur.comment = cur.raw;
        }
        else if(StringUtils::EndsWith(cur.raw, ":")) //label
        {
            cur.type = linelabel;
            cur.label = cur.raw.substr(0, cur.raw.length() - 1); //remove colon
            if((cur.label.length() == 0) || (cur.label == "\"\"")) //no label text
            {
                char message[256] = "";
                sprintf_s(message, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Empty label detected on line %d!")), i + 1);
                scriptError(0, message, gui);
                scriptLineMap.clear();
                return false;
            }
            int foundlabel = scriptLabelFind(cur.label);
            if(foundlabel) //label defined twice
            {
                char message[256] = "";
                sprintf_s(message, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Duplicate label \"%s\" detected on lines %d and %d!")), cur.label, foundlabel, i + 1);
                scriptError(0, message, gui);
                scriptLineMap.clear();
                return false;
            }
        }
        else if(scriptGetBranchType(cur.raw) != scriptnobranch) //branch
        {
            cur.type = linebranch;
            cur.branch.type = scriptGetBranchType(cur.raw);
            int rlen = cur.raw.length();
            for(int j = 0; j < rlen; j++)
                if(cur.raw[j] == ' ')
                {
                    String labelname = StringUtils::Trim(cur.raw.substr(j));
                    strcpy_s(cur.branch.branchlabel, labelname.c_str());
                    break;
                }
        }
        else
        {
            cur.type = linecommand;
            cur.command = cur.raw;
        }

        //append the comment to the raw line again
        if(line_comment.length())
            cur.raw += "\1" + line_comment;
        scriptLineMap.at(i) = cur;
    }
    linemapsize = (int)scriptLineMap.size();
    for(int i = 0; i < linemapsize; i++)
    {
        auto & currentLine = scriptLineMap.at(i);
        if(currentLine.type == linebranch) //invalid branch label
        {
            int labelline = scriptLabelFind(currentLine.branch.branchlabel);
            if(!labelline) //invalid branch label
            {
                char message[256] = "";
                sprintf_s(message, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Invalid branch label \"%s\" detected on line %d!")), currentLine.branch.branchlabel, i + 1);
                scriptError(0, message, gui);
                scriptLineMap.clear();
                return false;
            }
            else //set the branch destination line
                currentLine.branch.dest = scriptNextIp(labelline);
        }
    }
    // append a ret instruction at the end of the script when appropriate
    if(!scriptLineMap.empty())
    {
        entry = {};
        entry.type = linecommand;
        entry.raw = "ret";
        entry.command = "ret";

        const auto & lastline = scriptLineMap.back();
        switch(lastline.type)
        {
        case linecommand:
            if(scriptIsInternalCommand(lastline.command, "ret")
                    || scriptIsInternalCommand(lastline.command, "invalid")
                    || scriptIsInternalCommand(lastline.command, "error"))
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
            if(lastline.branch.type == scriptjmp)
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
            auto labelIp = scriptLabelFind(branchlabel);
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
    auto res = cmddirectexec(cmd);
    // Wait for the debuggee to pause
    while(bIsDebugging && dbgisrunning() && !bScriptAbort)
    {
        Sleep(1);
    }
    bScriptLogEnabled = false;
    if(!res)
        return STATUS_ERROR;
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
        scriptLastError = scriptInternalCmdExec(cur.command.c_str(), gui, state);
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
        if(scriptBranchTaken(cur.branch.type))
        {
            if(cur.branch.type == scriptcall) //calls have a special meaning
                scriptStack.push_back({scriptIp, state});
            scriptIp = scriptLabelFind(cur.branch.branchlabel);
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
    bScriptAbort = false;
    if(scriptIp)
        scriptIp--;
    scriptIp = scriptNextIp(scriptIp);
    bool bContinue = true;
    bool bIgnoreTimeout = settingboolget("Engine", "NoScriptTimeout", false);
    auto start = GetTickCount();
    while(bContinue && !bScriptAbort) //run loop
    {
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
    scriptState = SCRIPT_PAUSED; // set the script state to paused
    // re-enable GUI updates when appropriate
    if(disabledGuiUpdates)
        GuiUpdateEnable(true);
    GuiScriptSetIp(scriptIp);
    // the script fully executed (which means scriptIp is reset to the first line), without any errors
    return scriptIp == scriptNextIp(0) && (scriptLastError == STATUS_EXIT || scriptLastError == STATUS_CONTINUE || scriptLastError == STATUS_CONTINUE_BRANCH);
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
    bScriptAbort = false;
    if(!scriptCreateLineMap(filename, gui))
        return false; // Script load failed
    int lines = (int)scriptLineMap.size();
    auto script = (const char**)BridgeAlloc(lines * sizeof(const char*));
    for(int i = 0; i < lines; i++) //add script lines
        script[i] = scriptLineMap.at(i).raw.c_str();
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
        bScriptAbort = false;
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
            scriptState = SCRIPT_STEPPING;
            bRunGui = gui;
            scriptInternalCmd(gui, SCRIPT_STEPPING);
            scriptState = SCRIPT_PAUSED;
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

bool ScriptCmdExecAwait(const char* command, bool gui, const SCRIPTSTATE* abortState)
{
    // Capture the current script now, because this function might be called
    // through 'dbgcmdexecdirect("scriptcmd")' from a breakpoint command.
    // If we did not capture the script state it will be reset by the queue.
    // NOTE: Relevant if you step over 'erun' and a breakpoint with 'scriptcmd' is hit.
    auto state = abortState != nullptr ? *abortState : scriptState.load();
    return scriptQueue.await([command, state, gui]()
    {
        scriptIpOld = scriptIp;
        scriptLastError = scriptInternalCmdExec(command, gui, state);
        switch(scriptLastError)
        {
        case STATUS_ERROR:
            return false;
        case STATUS_EXIT:
        case STATUS_PAUSE:
        case STATUS_CONTINUE:
            break;
        case STATUS_CONTINUE_BRANCH:
            // If the user executes `scriptcmd call xxx`, start running.
            if(state == SCRIPT_PAUSED)
            {
                ScriptRunAsync(scriptIp, gui);
            }
            break;
        }
        return true;
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

SCRIPTABORTSTATE ScriptAbortAwait()
{
    auto state = scriptState.load();
    if(state != SCRIPT_PAUSED)
    {
        bScriptAbort = true;
        scriptQueue.await([]()
        {
            // Empty lambda just to wait for queue completion
        });
    }
    return { state, bRunGui };
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
    memcpy(info, &scriptLineMap.at(line - 1).branch, sizeof(SCRIPTBRANCH));
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
