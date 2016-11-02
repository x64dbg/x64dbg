/**
 @file x64dbg.cpp

 @brief Implements the 64 debug class.
 */

#include "_global.h"
#include "command.h"
#include "variable.h"
#include "debugger.h"
#include "simplescript.h"
#include "console.h"
#include "x64dbg.h"
#include "msgqueue.h"
#include "threading.h"
#include "watch.h"
#include "plugin_loader.h"
#include "_dbgfunctions.h"
#include <capstone_wrapper.h>
#include "_scriptapi_gui.h"
#include "filehelper.h"
#include "database.h"
#include "mnemonichelp.h"
#include "datainst_helper.h"
#include "exception.h"
#include "expressionfunctions.h"
#include "yara/yara.h"

static MESSAGE_STACK* gMsgStack = 0;
static HANDLE hCommandLoopThread = 0;
static bool bStopCommandLoopThread = false;
static char alloctrace[MAX_PATH] = "";
static bool bIsStopped = true;
static char scriptDllDir[MAX_PATH] = "";
static String notesFile;

static bool cbStrLen(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "not enough arguments!"));
        return false;
    }
    dprintf_untranslated("\"%s\"[%d]\n", argv[1], int(strlen(argv[1])));
    return true;
}

static bool cbClearLog(int argc, char* argv[])
{
    GuiLogClear();
    return true;
}

static bool cbPrintf(int argc, char* argv[])
{
    if(argc < 2)
        dprintf("\n");
    else
        dprintf("%s", argv[1]);
    return true;
}

static bool DbgScriptDllExec(const char* dll);

static bool cbScriptDll(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "not enough arguments!"));
        return false;
    }
    return DbgScriptDllExec(argv[1]);
}

#include "cmd-all.h"

/**
\brief register the all the commands
*/
static void registercommands()
{
    cmdinit();

    //general purpose
    dbgcmdnew("inc", cbInstrInc, false);
    dbgcmdnew("dec", cbInstrDec, false);
    dbgcmdnew("add", cbInstrAdd, false);
    dbgcmdnew("sub", cbInstrSub, false);
    dbgcmdnew("mul", cbInstrMul, false);
    dbgcmdnew("div", cbInstrDiv, false);
    dbgcmdnew("and", cbInstrAnd, false);
    dbgcmdnew("or", cbInstrOr, false);
    dbgcmdnew("xor", cbInstrXor, false);
    dbgcmdnew("neg", cbInstrNeg, false);
    dbgcmdnew("not", cbInstrNot, false);
    dbgcmdnew("bswap", cbInstrBswap, false);
    dbgcmdnew("rol", cbInstrRol, false);
    dbgcmdnew("ror", cbInstrRor, false);
    dbgcmdnew("shl\1sal", cbInstrShl, false);
    dbgcmdnew("shr", cbInstrShr, false);
    dbgcmdnew("sar", cbInstrSar, false);
    dbgcmdnew("push", cbInstrPush, true);
    dbgcmdnew("pop", cbInstrPop, true);
    dbgcmdnew("test", cbInstrTest, false);
    dbgcmdnew("cmp", cbInstrCmp, false);
    dbgcmdnew("mov\1set", cbInstrMov, false); //mov a variable, arg1:dest,arg2:src

    //debug control
    dbgcmdnew("InitDebug\1init\1initdbg", cbDebugInit, false); //init debugger arg1:exefile,[arg2:commandline]
    dbgcmdnew("StopDebug\1stop\1dbgstop", cbDebugStop, true); //stop debugger
    dbgcmdnew("AttachDebugger\1attach", cbDebugAttach, false); //attach
    dbgcmdnew("DetachDebugger\1detach", cbDebugDetach, true); //detach
    dbgcmdnew("run\1go\1r\1g", cbDebugRun, true); //unlock WAITID_RUN
    dbgcmdnew("erun\1egun\1er\1eg", cbDebugErun, true); //run + skip first chance exceptions
    dbgcmdnew("serun\1sego", cbDebugSerun, true); //run + swallow exception
    dbgcmdnew("pause", cbDebugPause, false); //pause debugger
    dbgcmdnew("DebugContinue\1con", cbDebugContinue, true); //set continue status
    dbgcmdnew("StepInto\1sti", cbDebugStepInto, true); //StepInto
    dbgcmdnew("eStepInto\1esti", cbDebugeStepInto, true); //StepInto + skip first chance exceptions
    dbgcmdnew("seStepInto\1sesti", cbDebugseStepInto, true); //StepInto + swallow exception
    dbgcmdnew("StepOver\1step\1sto\1st", cbDebugStepOver, true); //StepOver
    dbgcmdnew("eStepOver\1estep\1esto\1est", cbDebugeStepOver, true); //StepOver + skip first chance exceptions
    dbgcmdnew("seStepOver\1sestep\1sesto\1sest", cbDebugseStepOver, true); //StepOver + swallow exception
    dbgcmdnew("SingleStep\1sstep\1sst", cbDebugSingleStep, true); //SingleStep arg1:count
    dbgcmdnew("eSingleStep\1esstep\1esst", cbDebugeSingleStep, true); //SingleStep arg1:count + skip first chance exceptions
    dbgcmdnew("StepOut\1rtr", cbDebugStepOut, true); //StepOut
    dbgcmdnew("eStepOut\1ertr", cbDebugeStepOut, true); //rtr + skip first chance exceptions
    dbgcmdnew("skip", cbDebugSkip, true); //skip one instruction
    dbgcmdnew("InstrUndo", cbInstrInstrUndo, true); //Instruction undo

    //breakpoint control
    dbgcmdnew("SetBPX\1bp\1bpx", cbDebugSetBPX, true); //breakpoint
    dbgcmdnew("DeleteBPX\1bpc\1bc", cbDebugDeleteBPX, true); //breakpoint delete
    dbgcmdnew("EnableBPX\1bpe\1be", cbDebugEnableBPX, true); //breakpoint enable
    dbgcmdnew("DisableBPX\1bpd\1bd", cbDebugDisableBPX, true); //breakpoint disable
    dbgcmdnew("SetHardwareBreakpoint\1bph\1bphws", cbDebugSetHardwareBreakpoint, true); //hardware breakpoint
    dbgcmdnew("DeleteHardwareBreakpoint\1bphc\1bphwc", cbDebugDeleteHardwareBreakpoint, true); //delete hardware breakpoint
    dbgcmdnew("EnableHardwareBreakpoint\1bphe\1bphwe", cbDebugEnableHardwareBreakpoint, true); //enable hardware breakpoint
    dbgcmdnew("DisableHardwareBreakpoint\1bphd\1bphwd", cbDebugDisableHardwareBreakpoint, true); //disable hardware breakpoint
    dbgcmdnew("SetMemoryBPX\1membp\1bpm", cbDebugSetMemoryBpx, true); //SetMemoryBPX
    dbgcmdnew("DeleteMemoryBPX\1membpc\1bpmc", cbDebugDeleteMemoryBreakpoint, true); //delete memory breakpoint
    dbgcmdnew("EnableMemoryBreakpoint\1membpe\1bpme", cbDebugEnableMemoryBreakpoint, true); //enable memory breakpoint
    dbgcmdnew("DisableMemoryBreakpoint\1membpd\1bpmd", cbDebugDisableMemoryBreakpoint, true); //enable memory breakpoint
    dbgcmdnew("LibrarianSetBreakpoint\1bpdll", cbDebugBpDll, true); //set dll breakpoint
    dbgcmdnew("LibrarianRemoveBreakpoint\1bcdll", cbDebugBcDll, true); //remove dll breakpoint
    dbgcmdnew("LibrarianEnableBreakpoint\1bpedll", cbDebugBpDllEnable, true); //enable dll breakpoint
    dbgcmdnew("LibrarianDisableBreakpoint\1bpddll", cbDebugBpDllDisable, true); //disable dll breakpoint
    dbgcmdnew("SetExceptionBPX", cbDebugSetExceptionBPX, true); //set exception breakpoint
    dbgcmdnew("DeleteExceptionBPX", cbDebugDeleteExceptionBPX, true); //delete exception breakpoint
    dbgcmdnew("EnableExceptionBPX", cbDebugEnableExceptionBPX, true); //enable exception breakpoint
    dbgcmdnew("DisableExceptionBPX", cbDebugDisableExceptionBPX, true); //disable exception breakpoint
    dbgcmdnew("bpgoto", cbDebugSetBPGoto, true);
    dbgcmdnew("bplist", cbDebugBplist, true); //breakpoint list
    dbgcmdnew("SetBPXOptions\1bptype", cbDebugSetBPXOptions, false); //breakpoint type

    //conditional breakpoint control
    dbgcmdnew("SetBreakpointName\1bpname", cbDebugSetBPXName, true); //set breakpoint name
    dbgcmdnew("SetBreakpointCondition\1bpcond\1bpcnd", cbDebugSetBPXCondition, true); //set breakpoint breakCondition
    dbgcmdnew("SetBreakpointLog\1bplog\1bpl", cbDebugSetBPXLog, true); //set breakpoint logText
    dbgcmdnew("SetBreakpointLogCondition\1bplogcondition", cbDebugSetBPXLogCondition, true); //set breakpoint logCondition
    dbgcmdnew("SetBreakpointCommand", cbDebugSetBPXCommand, true); //set breakpoint command on hit
    dbgcmdnew("SetBreakpointCommandCondition", cbDebugSetBPXCommandCondition, true); //set breakpoint commandCondition
    dbgcmdnew("SetBreakpointFastResume", cbDebugSetBPXFastResume, true); //set breakpoint fast resume
    dbgcmdnew("SetBreakpointSingleshoot", cbDebugSetBPXSingleshoot, true); //set breakpoint singleshoot
    dbgcmdnew("SetBreakpointSilent", cbDebugSetBPXSilent, true); //set breakpoint fast resume
    dbgcmdnew("GetBreakpointHitCount", cbDebugGetBPXHitCount, true); //get breakpoint hit count
    dbgcmdnew("ResetBreakpointHitCount", cbDebugResetBPXHitCount, true); //reset breakpoint hit count

    dbgcmdnew("SetHardwareBreakpointName\1bphwname", cbDebugSetBPXHardwareName, true); //set breakpoint name
    dbgcmdnew("SetHardwareBreakpointCondition\1bphwcond", cbDebugSetBPXHardwareCondition, true); //set breakpoint breakCondition
    dbgcmdnew("SetHardwareBreakpointLog\1bphwlog", cbDebugSetBPXHardwareLog, true); //set breakpoint logText
    dbgcmdnew("SetHardwareBreakpointLogCondition\1bphwlogcondition", cbDebugSetBPXHardwareLogCondition, true); //set breakpoint logText
    dbgcmdnew("SetHardwareBreakpointCommand", cbDebugSetBPXHardwareCommand, true); //set breakpoint command on hit
    dbgcmdnew("SetHardwareBreakpointCommandCondition", cbDebugSetBPXHardwareCommandCondition, true); //set breakpoint commandCondition
    dbgcmdnew("SetHardwareBreakpointFastResume", cbDebugSetBPXHardwareFastResume, true); //set breakpoint fast resume
    dbgcmdnew("SetHardwareBreakpointSingleshoot", cbDebugSetBPXHardwareSingleshoot, true); //set breakpoint singleshoot
    dbgcmdnew("SetHardwareBreakpointSilent", cbDebugSetBPXHardwareSilent, true); //set breakpoint fast resume
    dbgcmdnew("GetHardwareBreakpointHitCount", cbDebugGetBPXHardwareHitCount, true); //get breakpoint hit count
    dbgcmdnew("ResetHardwareBreakpointHitCount", cbDebugResetBPXHardwareHitCount, true); //reset breakpoint hit count

    dbgcmdnew("SetMemoryBreakpointName\1bpmname", cbDebugSetBPXMemoryName, true); //set breakpoint name
    dbgcmdnew("SetMemoryBreakpointCondition\1bpmcond", cbDebugSetBPXMemoryCondition, true); //set breakpoint breakCondition
    dbgcmdnew("SetMemoryBreakpointLog\1bpmlog", cbDebugSetBPXMemoryLog, true); //set breakpoint log
    dbgcmdnew("SetMemoryBreakpointLogCondition\1bpmlogcondition", cbDebugSetBPXMemoryLogCondition, true); //set breakpoint logCondition
    dbgcmdnew("SetMemoryBreakpointCommand", cbDebugSetBPXMemoryCommand, true); //set breakpoint command on hit
    dbgcmdnew("SetMemoryBreakpointCommandCondition", cbDebugSetBPXMemoryCommandCondition, true); //set breakpoint commandCondition
    dbgcmdnew("SetMemoryBreakpointFastResume", cbDebugSetBPXMemoryFastResume, true); //set breakpoint fast resume
    dbgcmdnew("SetMemoryBreakpointSingleshoot", cbDebugSetBPXMemorySingleshoot, true); //set breakpoint singleshoot
    dbgcmdnew("SetMemoryBreakpointSilent", cbDebugSetBPXMemorySilent, true); //set breakpoint fast resume
    dbgcmdnew("GetMemoryBreakpointHitCount", cbDebugGetBPXMemoryHitCount, true); //get breakpoint hit count
    dbgcmdnew("ResetMemoryBreakpointHitCount", cbDebugResetBPXMemoryHitCount, true); //reset breakpoint hit count

    dbgcmdnew("SetLibrarianBreakpointName", cbDebugSetBPXDLLName, true); //set breakpoint name
    dbgcmdnew("SetLibrarianBreakpointCondition", cbDebugSetBPXDLLCondition, true); //set breakpoint breakCondition
    dbgcmdnew("SetLibrarianBreakpointLog", cbDebugSetBPXDLLLog, true); //set breakpoint log
    dbgcmdnew("SetLibrarianBreakpointLogCondition", cbDebugSetBPXDLLLogCondition, true); //set breakpoint logCondition
    dbgcmdnew("SetLibrarianBreakpointCommand", cbDebugSetBPXDLLCommand, true); //set breakpoint command on hit
    dbgcmdnew("SetLibrarianBreakpointCommandCondition", cbDebugSetBPXDLLCommandCondition, true); //set breakpoint commandCondition
    dbgcmdnew("SetLibrarianBreakpointFastResume", cbDebugSetBPXDLLFastResume, true); //set breakpoint fast resume
    dbgcmdnew("SetLibrarianBreakpointSingleshoot", cbDebugSetBPXDLLSingleshoot, true); //set breakpoint singleshoot
    dbgcmdnew("SetLibrarianBreakpointSilent", cbDebugSetBPXDLLSilent, true); //set breakpoint fast resume
    dbgcmdnew("GetLibrarianBreakpointHitCount", cbDebugGetBPXDLLHitCount, true); //get breakpoint hit count
    dbgcmdnew("ResetLibrarianBreakpointHitCount", cbDebugResetBPXDLLHitCount, true); //reset breakpoint hit count

    dbgcmdnew("SetExceptionBreakpointName", cbDebugSetBPXExceptionName, true); //set breakpoint name
    dbgcmdnew("SetExceptionBreakpointCondition", cbDebugSetBPXExceptionCondition, true); //set breakpoint breakCondition
    dbgcmdnew("SetExceptionBreakpointLog", cbDebugSetBPXExceptionLog, true); //set breakpoint log
    dbgcmdnew("SetExceptionBreakpointLogCondition", cbDebugSetBPXExceptionLogCondition, true); //set breakpoint logCondition
    dbgcmdnew("SetExceptionBreakpointCommand", cbDebugSetBPXExceptionCommand, true); //set breakpoint command on hit
    dbgcmdnew("SetExceptionBreakpointCommandCondition", cbDebugSetBPXExceptionCommandCondition, true); //set breakpoint commandCondition
    dbgcmdnew("SetExceptionBreakpointFastResume", cbDebugSetBPXExceptionFastResume, true); //set breakpoint fast resume
    dbgcmdnew("SetExceptionBreakpointSingleshoot", cbDebugSetBPXExceptionSingleshoot, true); //set breakpoint singleshoot
    dbgcmdnew("SetExceptionBreakpointSilent", cbDebugSetBPXExceptionSilent, true); //set breakpoint fast resume
    dbgcmdnew("GetExceptionBreakpointHitCount", cbDebugGetBPXExceptionHitCount, true); //get breakpoint hit count
    dbgcmdnew("ResetExceptionBreakpointHitCount", cbDebugResetBPXExceptionHitCount, true); //reset breakpoint hit count

    //tracing
    dbgcmdnew("TraceIntoConditional\1ticnd", cbDebugTraceIntoConditional, true); //Trace into conditional
    dbgcmdnew("TraceOverConditional\1tocnd", cbDebugTraceOverConditional, true); //Trace over conditional
    dbgcmdnew("TraceIntoBeyondTraceRecord\1tibt", cbDebugTraceIntoBeyondTraceRecord, true); //Trace into beyond trace record
    dbgcmdnew("TraceOverBeyondTraceRecord\1tobt", cbDebugTraceOverBeyondTraceRecord, true); //Trace over beyond trace record
    dbgcmdnew("TraceIntoIntoTraceRecord\1tiit", cbDebugTraceIntoIntoTraceRecord, true); //Trace into into trace record
    dbgcmdnew("TraceOverIntoTraceRecord\1toit", cbDebugTraceOverIntoTraceRecord, true); //Trace over into trace record
    dbgcmdnew("RunToParty", cbDebugRunToParty, true); //Run to code in a party
    dbgcmdnew("RunToUserCode\1rtu", cbDebugRunToUserCode, true); //Run to user code
    dbgcmdnew("TraceSetLog\1SetTraceLog", cbDebugTraceSetLog, true); //Set trace log text + condition
    dbgcmdnew("TraceSetCommand\1SetTraceCommand", cbDebugTraceSetCommand, true); //Set trace command text + condition

    //thread control
    dbgcmdnew("createthread\1threadcreate\1newthread\1threadnew", cbDebugCreatethread, true); //create thread
    dbgcmdnew("switchthread\1threadswitch", cbDebugSwitchthread, true); //switch thread
    dbgcmdnew("suspendthread\1threadsuspend", cbDebugSuspendthread, true); //suspend thread
    dbgcmdnew("resumethread\1threadresume", cbDebugResumethread, true); //resume thread
    dbgcmdnew("killthread\1threadkill", cbDebugKillthread, true); //kill thread
    dbgcmdnew("suspendallthreads\1threadsuspendall", cbDebugSuspendAllThreads, true); //suspend all threads
    dbgcmdnew("resumeallthreads\1threadresumeall", cbDebugResumeAllThreads, true); //resume all threads
    dbgcmdnew("setthreadpriority\1setprioritythread\1threadsetpriority", cbDebugSetPriority, true); //set thread priority
    dbgcmdnew("threadsetname\1setthreadname", cbDebugSetthreadname, true); //set thread name

    //memory operations
    dbgcmdnew("alloc", cbDebugAlloc, true); //allocate memory
    dbgcmdnew("free", cbDebugFree, true); //free memory
    dbgcmdnew("Fill\1memset", cbDebugMemset, true); //memset
    dbgcmdnew("getpagerights\1getrightspage", cbDebugGetPageRights, true);
    dbgcmdnew("setpagerights\1setrightspage", cbDebugSetPageRights, true);
    dbgcmdnew("savedata", cbInstrSavedata, true); //save data to disk

    //operating system control
    dbgcmdnew("GetPrivilegeState", cbGetPrivilegeState, true); //get priv state
    dbgcmdnew("EnablePrivilege", cbEnablePrivilege, true); //enable priv
    dbgcmdnew("DisablePrivilege", cbDisablePrivilege, true); //disable priv
    dbgcmdnew("handleclose\1closehandle", cbHandleClose, true); //close remote handle

    //watch control
    dbgcmdnew("AddWatch", cbAddWatch, true); // add watch
    dbgcmdnew("DelWatch", cbDelWatch, true); // delete watch
    dbgcmdnew("SetWatchdog", cbSetWatchdog, true); // Setup watchdog
    dbgcmdnew("SetWatchExpression", cbSetWatchExpression, true); // Set watch expression
    dbgcmdnew("SetWatchName", cbSetWatchName, true); // Set watch name
    dbgcmdnew("CheckWatchdog", cbCheckWatchdog, true); // Watchdog

    //variables
    dbgcmdnew("varnew\1var", cbInstrVar, false); //make a variable arg1:name,[arg2:value]
    dbgcmdnew("vardel", cbInstrVarDel, false); //delete a variable, arg1:variable name
    dbgcmdnew("varlist", cbInstrVarList, false); //list variables[arg1:type filter]

    //searching
    dbgcmdnew("find", cbInstrFind, true); //find a pattern
    dbgcmdnew("findall", cbInstrFindAll, true); //find all patterns
    dbgcmdnew("findallmem\1findmemall", cbInstrFindAllMem, true); //memory map pattern find
    dbgcmdnew("findasm\1asmfind", cbInstrFindAsm, true); //find instruction
    dbgcmdnew("reffind\1findref\1ref", cbInstrRefFind, true); //find references to a value
    dbgcmdnew("reffindrange\1findrefrange\1refrange", cbInstrRefFindRange, true);
    dbgcmdnew("refstr\1strref", cbInstrRefStr, true); //find string references
    dbgcmdnew("modcallfind", cbInstrModCallFind, true); //find intermodular calls
    dbgcmdnew("yara", cbInstrYara, true); //yara test command
    dbgcmdnew("yaramod", cbInstrYaramod, true); //yara rule on module
    dbgcmdnew("setmaxfindresult\1findsetmaxresult", cbInstrSetMaxFindResult, false); //set the maximum number of occurences found
    dbgcmdnew("guidfind\1findguid", cbInstrGUIDFind, true); //find GUID references TODO: undocumented

    //user database
    dbgcmdnew("dbsave\1savedb", cbInstrDbsave, true); //save program database
    dbgcmdnew("dbload\1loaddb", cbInstrDbload, true); //load program database
    dbgcmdnew("dbclear\1cleardb", cbInstrDbclear, true); //clear program database

    dbgcmdnew("commentset\1cmt\1cmtset", cbInstrCommentSet, true); //set/edit comment
    dbgcmdnew("commentdel\1cmtc\1cmtdel", cbInstrCommentDel, true); //delete comment
    dbgcmdnew("commentlist", cbInstrCommentList, true); //list comments
    dbgcmdnew("commentclear", cbInstrCommentClear, true); //clear comments

    dbgcmdnew("labelset\1lbl\1lblset", cbInstrLabelSet, true); //set/edit label
    dbgcmdnew("labeldel\1lblc\1lbldel", cbInstrLabelDel, true); //delete label
    dbgcmdnew("labellist", cbInstrLabelList, true); //list labels
    dbgcmdnew("labelclear", cbInstrLabelClear, true); //clear labels

    dbgcmdnew("bookmarkset\1bookmark", cbInstrBookmarkSet, true); //set bookmark
    dbgcmdnew("bookmarkdel\1bookmarkc", cbInstrBookmarkDel, true); //delete bookmark
    dbgcmdnew("bookmarklist", cbInstrBookmarkList, true); //list bookmarks
    dbgcmdnew("bookmarkclear", cbInstrBookmarkClear, true); //clear bookmarks

    dbgcmdnew("functionadd\1func", cbInstrFunctionAdd, true); //function
    dbgcmdnew("functiondel\1funcc", cbInstrFunctionDel, true); //function
    dbgcmdnew("functionlist", cbInstrFunctionList, true); //list functions
    dbgcmdnew("functionclear", cbInstrFunctionClear, false); //delete all functions

    dbgcmdnew("argumentadd", cbInstrArgumentAdd, true); //add argument
    dbgcmdnew("argumentdel", cbInstrArgumentDel, true); //delete argument
    dbgcmdnew("argumentlist", cbInstrArgumentList, true); //list arguments
    dbgcmdnew("argumentclear", cbInstrArgumentClear, false); //delete all arguments

    //analysis
    dbgcmdnew("analyse\1analyze\1anal", cbInstrAnalyse, true); //secret analysis command
    dbgcmdnew("exanal\1exanalyse\1exanalyze", cbInstrExanalyse, true); //exception directory analysis
    dbgcmdnew("cfanal\1cfanalyse\1cfanalyze", cbInstrCfanalyse, true); //control flow analysis
    dbgcmdnew("analyse_nukem\1analyze_nukem\1anal_nukem", cbInstrAnalyseNukem, true); //secret analysis command #2
    dbgcmdnew("analxrefs\1analx", cbInstrAnalxrefs, true); //analyze xrefs
    dbgcmdnew("analrecur\1analr", cbInstrAnalrecur, true); //analyze a single function
    dbgcmdnew("analadv", cbInstrAnalyseadv, true); //analyze xref,function and data
    dbgcmdnew("traceexecute", cbInstrTraceexecute, true); //execute trace record on address TODO: undocumented

    dbgcmdnew("virtualmod", cbInstrVirtualmod, true); //virtual module
    dbgcmdnew("symdownload\1downloadsym", cbDebugDownloadSymbol, true); //download symbols
    dbgcmdnew("imageinfo\1modimageinfo", cbInstrImageinfo, true); //print module image information
    dbgcmdnew("GetRelocSize\1grs", cbInstrGetRelocSize, true); //get relocation table size
    dbgcmdnew("exhandlers", cbInstrExhandlers, true); //enumerate exception handlers
    dbgcmdnew("exinfo", cbInstrExinfo, true); //dump last exception information

    //types
    dbgcmdnew("DataUnknown", cbInstrDataUnknown, true); //mark as Unknown
    dbgcmdnew("DataByte\1db", cbInstrDataByte, true); //mark as Byte
    dbgcmdnew("DataWord\1dw", cbInstrDataWord, true); //mark as Word
    dbgcmdnew("DataDword\1dd", cbInstrDataDword, true); //mark as Dword
    dbgcmdnew("DataFword", cbInstrDataFword, true); //mark as Fword
    dbgcmdnew("DataQword\1dq", cbInstrDataQword, true); //mark as Qword
    dbgcmdnew("DataTbyte", cbInstrDataTbyte, true); //mark as Tbyte
    dbgcmdnew("DataOword", cbInstrDataOword, true); //mark as Oword
    dbgcmdnew("DataMmword", cbInstrDataMmword, true); //mark as Mmword
    dbgcmdnew("DataXmmword", cbInstrDataXmmword, true); //mark as Xmmword
    dbgcmdnew("DataYmmword", cbInstrDataYmmword, true); //mark as Ymmword
    dbgcmdnew("DataFloat\1DataReal4\1df", cbInstrDataFloat, true); //mark as Float
    dbgcmdnew("DataDouble\1DataReal8", cbInstrDataDouble, true); //mark as Double
    dbgcmdnew("DataLongdouble\1DataReal10", cbInstrDataLongdouble, true); //mark as Longdouble
    dbgcmdnew("DataAscii\1da", cbInstrDataAscii, true); //mark as Ascii
    dbgcmdnew("DataUnicode\1du", cbInstrDataUnicode, true); //mark as Unicode
    dbgcmdnew("DataCode\1dc", cbInstrDataCode, true); //mark as Code
    dbgcmdnew("DataJunk", cbInstrDataJunk, true); //mark as Junk
    dbgcmdnew("DataMiddle", cbInstrDataMiddle, true); //mark as Middle

    dbgcmdnew("AddType", cbInstrAddType, false); //AddType
    dbgcmdnew("AddStruct", cbInstrAddStruct, false); //AddStruct
    dbgcmdnew("AddUnion", cbInstrAddUnion, false); //AddUnion
    dbgcmdnew("AddMember", cbInstrAddMember, false); //AddMember
    dbgcmdnew("AppendMember", cbInstrAppendMember, false); //AppendMember
    dbgcmdnew("AddFunction", cbInstrAddFunction, false); //AddFunction
    dbgcmdnew("AddArg", cbInstrAddArg, false); //AddArg
    dbgcmdnew("AppendArg", cbInstrAppendArg, false); //AppendArg
    dbgcmdnew("SizeofType", cbInstrSizeofType, false); //SizeofType
    dbgcmdnew("VisitType", cbInstrVisitType, false); //VisitType
    dbgcmdnew("ClearTypes", cbInstrClearTypes, false); //ClearTypes
    dbgcmdnew("RemoveType", cbInstrRemoveType, false); //RemoveType
    dbgcmdnew("EnumTypes", cbInstrEnumTypes, false); //EnumTypes

    //plugins
    dbgcmdnew("StartScylla\1scylla\1imprec", cbDebugStartScylla, false); //start scylla
    dbgcmdnew("plugload\1pluginload\1loadplugin", cbInstrPluginLoad, false); //load plugin
    dbgcmdnew("plugunload\1pluginunload\1unloadplugin", cbInstrPluginUnload, false); //unload plugin

    //script
    dbgcmdnew("scriptload", cbScriptLoad, false);
    dbgcmdnew("msg", cbScriptMsg, false);
    dbgcmdnew("msgyn", cbScriptMsgyn, false);
    dbgcmdnew("log", cbInstrLog, false); //log command with superawesome hax
    dbgcmdnew("scriptdll\1dllscript", cbScriptDll, false); //execute a script DLL

    //gui
    dbgcmdnew("disasm\1dis\1d", cbDebugDisasm, true); //doDisasm
    dbgcmdnew("dump", cbDebugDump, true); //dump at address
    dbgcmdnew("sdump", cbDebugStackDump, true); //dump at stack address
    dbgcmdnew("memmapdump", cbDebugMemmapdump, true);
    dbgcmdnew("graph", cbInstrGraph, true); //graph function
    dbgcmdnew("guiupdateenable", cbInstrEnableGuiUpdate, true); //enable gui message
    dbgcmdnew("guiupdatedisable", cbInstrDisableGuiUpdate, true); //disable gui message
    dbgcmdnew("setfreezestack", cbDebugSetfreezestack, false); //freeze the stack from auto updates
    dbgcmdnew("refinit", cbInstrRefinit, false);
    dbgcmdnew("refadd", cbInstrRefadd, false);
    dbgcmdnew("EnableLog\1LogEnable", cbInstrEnableLog, false); //enable log
    dbgcmdnew("DisableLog\1LogDisable", cbInstrDisableLog, false); //disable log
    dbgcmdnew("ClearLog\1cls\1lc\1lclr", cbClearLog, false); //clear the log
    dbgcmdnew("AddFavouriteTool", cbInstrAddFavTool, false); //add favourite tool
    dbgcmdnew("AddFavouriteCommand", cbInstrAddFavCmd, false); //add favourite command
    dbgcmdnew("AddFavouriteToolShortcut\1SetFavouriteToolShortcut", cbInstrSetFavToolShortcut, false); //set favourite tool shortcut
    dbgcmdnew("FoldDisassembly", cbInstrFoldDisassembly, true); //fold disassembly segment

    //misc
    dbgcmdnew("chd", cbInstrChd, false); //Change directory
    dbgcmdnew("zzz\1doSleep", cbInstrZzz, false); //sleep

    dbgcmdnew("HideDebugger\1dbh\1hide", cbDebugHide, true); //HideDebugger
    dbgcmdnew("loadlib", cbDebugLoadLib, true); //Load DLL
    dbgcmdnew("asm", cbInstrAssemble, true); //assemble instruction
    dbgcmdnew("gpa", cbInstrGpa, true); //get proc address

    dbgcmdnew("setjit\1jitset", cbDebugSetJIT, false); //set JIT
    dbgcmdnew("getjit\1jitget", cbDebugGetJIT, false); //get JIT
    dbgcmdnew("getjitauto\1jitgetauto", cbDebugGetJITAuto, false); //get JIT Auto
    dbgcmdnew("setjitauto\1jitsetauto", cbDebugSetJITAuto, false); //set JIT Auto

    dbgcmdnew("getcommandline\1getcmdline", cbDebugGetCmdline, true); //Get CmdLine
    dbgcmdnew("setcommandline\1setcmdline", cbDebugSetCmdline, true); //Set CmdLine

    dbgcmdnew("mnemonichelp", cbInstrMnemonichelp, false); //mnemonic help
    dbgcmdnew("mnemonicbrief", cbInstrMnemonicbrief, false); //mnemonic brief

    //undocumented
    dbgcmdnew("bench", cbDebugBenchmark, true); //benchmark test (readmem etc)
    dbgcmdnew("dprintf", cbPrintf, false); //printf
    dbgcmdnew("setstr\1strset", cbInstrSetstr, false); //set a string variable
    dbgcmdnew("getstr\1strget", cbInstrGetstr, false); //get a string variable
    dbgcmdnew("copystr\1strcpy", cbInstrCopystr, true); //write a string variable to memory
    dbgcmdnew("looplist", cbInstrLoopList, true); //list loops
    dbgcmdnew("capstone", cbInstrCapstone, true); //disassemble using capstone
    dbgcmdnew("visualize", cbInstrVisualize, true); //visualize analysis
    dbgcmdnew("meminfo", cbInstrMeminfo, true); //command to debug memory map bugs
    dbgcmdnew("briefcheck", cbInstrBriefcheck, true); //check if mnemonic briefs are missing
    dbgcmdnew("focusinfo", cbInstrFocusinfo, false);
    dbgcmdnew("printstack\1logstack", cbInstrPrintStack, true); //print the call stack
};

bool cbCommandProvider(char* cmd, int maxlen)
{
    MESSAGE msg;
    MsgWait(gMsgStack, &msg);
    if(bStopCommandLoopThread)
        return false;
    char* newcmd = (char*)msg.param1;
    if(strlen(newcmd) >= deflen)
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "command cut at ~%d characters\n"), deflen);
        newcmd[deflen - 2] = 0;
    }
    strcpy_s(cmd, deflen, newcmd);
    efree(newcmd, "cbCommandProvider:newcmd"); //free allocated command
    return true;
}

/**
\brief Execute command asynchronized.
*/
extern "C" DLL_EXPORT bool _dbg_dbgcmdexec(const char* cmd)
{
    int len = (int)strlen(cmd);
    char* newcmd = (char*)emalloc((len + 1) * sizeof(char), "_dbg_dbgcmdexec:newcmd");
    strcpy_s(newcmd, len + 1, cmd);
    return MsgSend(gMsgStack, 0, (duint)newcmd, 0);
}

static DWORD WINAPI DbgCommandLoopThread(void* a)
{
    cmdloop();
    return 0;
}

typedef void(*SCRIPTDLLSTART)();

struct DLLSCRIPTEXECTHREADINFO
{
    DLLSCRIPTEXECTHREADINFO(HINSTANCE hScriptDll, SCRIPTDLLSTART AsyncStart)
        : hScriptDll(hScriptDll),
          AsyncStart(AsyncStart)
    {
    }

    HINSTANCE hScriptDll;
    SCRIPTDLLSTART AsyncStart;
};

static DWORD WINAPI DbgScriptDllExecThread(void* a)
{
    auto info = (DLLSCRIPTEXECTHREADINFO*)a;
    auto AsyncStart = info->AsyncStart;
    auto hScriptDll = info->hScriptDll;
    delete info;

    dputs(QT_TRANSLATE_NOOP("DBG", "[Script DLL] Calling export \"AsyncStart\"...\n"));
    AsyncStart();
    dputs(QT_TRANSLATE_NOOP("DBG", "[Script DLL] \"AsyncStart\" returned!\n"));

    dputs(QT_TRANSLATE_NOOP("DBG", "[Script DLL] Calling FreeLibrary..."));
    if(FreeLibrary(hScriptDll))
        dputs(QT_TRANSLATE_NOOP("DBG", "success!\n"));
    else
        dprintf(QT_TRANSLATE_NOOP("DBG", "failure (%08X)...\n"), GetLastError());

    return 0;
}

static bool DbgScriptDllExec(const char* dll)
{
    String dllPath = dll;
    if(dllPath.find('\\') == String::npos)
        dllPath = String(scriptDllDir) + String(dll);

    dprintf(QT_TRANSLATE_NOOP("DBG", "[Script DLL] Loading Script DLL \"%s\"...\n"), dllPath.c_str());

    auto hScriptDll = LoadLibraryW(StringUtils::Utf8ToUtf16(dllPath).c_str());
    if(hScriptDll)
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "[Script DLL] DLL loaded on 0x%p!\n"), hScriptDll);

        auto AsyncStart = SCRIPTDLLSTART(GetProcAddress(hScriptDll, "AsyncStart"));
        if(AsyncStart)
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "[Script DLL] Creating thread to call the export \"AsyncStart\"...\n"));
            CloseHandle(CreateThread(nullptr, 0, DbgScriptDllExecThread, new DLLSCRIPTEXECTHREADINFO(hScriptDll, AsyncStart), 0, nullptr)); //on-purpose memory leak here
        }
        else
        {
            auto Start = SCRIPTDLLSTART(GetProcAddress(hScriptDll, "Start"));
            if(Start)
            {
                dputs(QT_TRANSLATE_NOOP("DBG", "[Script DLL] Calling export \"Start\"...\n"));
                Start();
                dputs(QT_TRANSLATE_NOOP("DBG", "[Script DLL] \"Start\" returned!\n"));
            }
            else
                dprintf(QT_TRANSLATE_NOOP("DBG", "[Script DLL] Failed to find the exports \"AsyncStart\" or \"Start\" (%s)!\n"), ErrorCodeToName(GetLastError()).c_str());

            dprintf(QT_TRANSLATE_NOOP("DBG", "[Script DLL] Calling FreeLibrary..."));
            if(FreeLibrary(hScriptDll))
                dputs(QT_TRANSLATE_NOOP("DBG", "success!\n"));
            else
                dprintf(QT_TRANSLATE_NOOP("DBG", "failure (%s)...\n"), ErrorCodeToName(GetLastError()).c_str());
        }
    }
    else
        dprintf(QT_TRANSLATE_NOOP("DBG", "[Script DLL] LoadLibary failed (%s)!\n"), ErrorCodeToName(GetLastError()).c_str());

    return true;
}

static DWORD WINAPI loadDbThread(LPVOID)
{
    // Load mnemonic help database
    String mnemonicHelpData;
    if(FileHelper::ReadAllText(StringUtils::sprintf("%s\\..\\mnemdb.json", szProgramDir), mnemonicHelpData))
    {
        if(MnemonicHelp::loadFromText(mnemonicHelpData.c_str()))
            dputs(QT_TRANSLATE_NOOP("DBG", "Mnemonic help database loaded!"));
        else
            dputs(QT_TRANSLATE_NOOP("DBG", "Failed to load mnemonic help database..."));
    }
    else
        dputs(QT_TRANSLATE_NOOP("DBG", "Failed to read mnemonic help database..."));

    // Load error codes
    if(ErrorCodeInit(StringUtils::sprintf("%s\\..\\errordb.txt", szProgramDir)))
        dputs(QT_TRANSLATE_NOOP("DBG", "Error codes database loaded!"));
    else
        dputs(QT_TRANSLATE_NOOP("DBG", "Failed to load error codes..."));

    // Load exception codes
    if(ExceptionCodeInit(StringUtils::sprintf("%s\\..\\exceptiondb.txt", szProgramDir)))
        dputs(QT_TRANSLATE_NOOP("DBG", "Exception codes database loaded!"));
    else
        dputs(QT_TRANSLATE_NOOP("DBG", "Failed to load exception codes..."));

    // Load NTSTATUS codes
    if(NtStatusCodeInit(StringUtils::sprintf("%s\\..\\ntstatusdb.txt", szProgramDir)))
        dputs(QT_TRANSLATE_NOOP("DBG", "NTSTATUS codes database loaded!"));
    else
        dputs(QT_TRANSLATE_NOOP("DBG", "Failed to load NTSTATUS codes..."));

    // Load global notes
    dputs(QT_TRANSLATE_NOOP("DBG", "Reading notes file..."));
    notesFile = String(szProgramDir) + "\\notes.txt";
    String text;
    if(FileHelper::ReadAllText(notesFile, text))
        GuiSetGlobalNotes(text.c_str());
    else
        dputs(QT_TRANSLATE_NOOP("DBG", "Reading notes failed..."));

    dputs(QT_TRANSLATE_NOOP("DBG", "File read thread finished!"));

    return 0;
}

extern "C" DLL_EXPORT const char* _dbg_dbginit()
{
    if(!EngineCheckStructAlignment(UE_STRUCT_TITAN_ENGINE_CONTEXT, sizeof(TITAN_ENGINE_CONTEXT_t)))
        return "Invalid TITAN_ENGINE_CONTEXT_t alignment!";

    static_assert(sizeof(TITAN_ENGINE_CONTEXT_t) == sizeof(REGISTERCONTEXT), "Invalid REGISTERCONTEXT alignment!");

    dputs(QT_TRANSLATE_NOOP("DBG", "Initializing wait objects..."));
    waitinitialize();
    dputs(QT_TRANSLATE_NOOP("DBG", "Initializing debugger..."));
    dbginit();
    dputs(QT_TRANSLATE_NOOP("DBG", "Initializing debugger functions..."));
    dbgfunctionsinit();
#ifdef ENABLE_MEM_TRACE
    dputs(QT_TRANSLATE_NOOP("DBG", "Setting JSON memory management functions..."));
    json_set_alloc_funcs(json_malloc, json_free);
#endif //ENABLE_MEM_TRACE
    dputs(QT_TRANSLATE_NOOP("DBG", "Initializing capstone..."));
    Capstone::GlobalInitialize();
    dputs(QT_TRANSLATE_NOOP("DBG", "Initializing Yara..."));
    if(yr_initialize() != ERROR_SUCCESS)
        return "Failed to initialize Yara!";
    dputs(QT_TRANSLATE_NOOP("DBG", "Getting directory information..."));
    wchar_t wszDir[deflen] = L"";
    if(!GetModuleFileNameW(hInst, wszDir, deflen))
        return "GetModuleFileNameW failed!";
    strcpy_s(szProgramDir, StringUtils::Utf16ToUtf8(wszDir).c_str());
    int len = (int)strlen(szProgramDir);
    while(szProgramDir[len] != '\\')
        len--;
    szProgramDir[len] = 0;
#ifdef ENABLE_MEM_TRACE
    strcpy_s(alloctrace, szProgramDir);
    strcat_s(alloctrace, "\\alloctrace.txt");
    DeleteFileW(StringUtils::Utf8ToUtf16(alloctrace).c_str());
    setalloctrace(alloctrace);
#endif //ENABLE_MEM_TRACE
    strcpy_s(scriptDllDir, szProgramDir);
    strcat_s(scriptDllDir, "\\scripts\\");
    initDataInstMap();

    dputs(QT_TRANSLATE_NOOP("DBG", "Start file read thread..."));
    CloseHandle(CreateThread(nullptr, 0, loadDbThread, nullptr, 0, nullptr));

    // Create database directory in the local debugger folder
    DbSetPath(StringUtils::sprintf("%s\\db", szProgramDir).c_str(), nullptr);

    char szLocalSymbolPath[MAX_PATH] = "";
    strcpy_s(szLocalSymbolPath, szProgramDir);
    strcat_s(szLocalSymbolPath, "\\symbols");

    Memory<char*> cachePath(MAX_SETTING_SIZE + 1);
    if(!BridgeSettingGet("Symbols", "CachePath", cachePath()) || !*cachePath())
    {
        strcpy_s(szSymbolCachePath, szLocalSymbolPath);
        BridgeSettingSet("Symbols", "CachePath", ".\\symbols");
    }
    else
    {
        if(_strnicmp(cachePath(), ".\\", 2) == 0)
        {
            strncpy_s(szSymbolCachePath, szProgramDir, _TRUNCATE);
            strncat_s(szSymbolCachePath, cachePath() + 1, _TRUNCATE);
        }
        else
        {
            // Trim the buffer to fit inside MAX_PATH
            strncpy_s(szSymbolCachePath, cachePath(), _TRUNCATE);
        }

        if(strstr(szSymbolCachePath, "http://") || strstr(szSymbolCachePath, "https://"))
        {
            if(Script::Gui::MessageYesNo(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "It is strongly discouraged to use symbol servers in your path directly (use the store option instead).\n\nDo you want me to fix this?"))))
            {
                strcpy_s(szSymbolCachePath, szLocalSymbolPath);
                BridgeSettingSet("Symbols", "CachePath", ".\\symbols");
            }
        }
    }
    dprintf(QT_TRANSLATE_NOOP("DBG", "Symbol Path: %s\n"), szSymbolCachePath);
    SetCurrentDirectoryW(StringUtils::Utf8ToUtf16(szProgramDir).c_str());
    dputs(QT_TRANSLATE_NOOP("DBG", "Allocating message stack..."));
    gMsgStack = MsgAllocStack();
    if(!gMsgStack)
        return "Could not allocate message stack!";
    dputs(QT_TRANSLATE_NOOP("DBG", "Initializing global script variables..."));
    varinit();
    dputs(QT_TRANSLATE_NOOP("DBG", "Registering debugger commands..."));
    registercommands();
    dputs(QT_TRANSLATE_NOOP("DBG", "Registering GUI command handler..."));
    ExpressionFunctions::Init();
    dputs(QT_TRANSLATE_NOOP("DBG", "Registering expression functions..."));
    SCRIPTTYPEINFO info;
    strcpy_s(info.name, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Default")));
    info.id = 0;
    info.execute = DbgCmdExec;
    info.completeCommand = nullptr;
    GuiRegisterScriptLanguage(&info);
    dputs(QT_TRANSLATE_NOOP("DBG", "Registering Script DLL command handler..."));
    strcpy_s(info.name, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Script DLL")));
    info.execute = DbgScriptDllExec;
    GuiRegisterScriptLanguage(&info);
    dputs(QT_TRANSLATE_NOOP("DBG", "Starting command loop..."));
    hCommandLoopThread = CreateThread(nullptr, 0, DbgCommandLoopThread, nullptr, 0, nullptr);
    char plugindir[deflen] = "";
    strcpy_s(plugindir, szProgramDir);
    strcat_s(plugindir, "\\plugins");
    CreateDirectoryW(StringUtils::Utf8ToUtf16(plugindir).c_str(), nullptr);
    CreateDirectoryW(StringUtils::Utf8ToUtf16(StringUtils::sprintf("%s\\memdumps", szProgramDir)).c_str(), nullptr);
    dputs(QT_TRANSLATE_NOOP("DBG", "Loading plugins..."));
    pluginloadall(plugindir);
    dputs(QT_TRANSLATE_NOOP("DBG", "Handling command line..."));
    //handle command line
    int argc = 0;
    wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if(argc == 2) //1 argument (init filename)
        DbgCmdExec(StringUtils::Utf16ToUtf8(StringUtils::sprintf(L"init \"%s\"", argv[1])).c_str());
    else if(argc == 3)  //2 arguments (init filename, cmdline)
        DbgCmdExec(StringUtils::Utf16ToUtf8(StringUtils::sprintf(L"init \"%s\", \"%s\"", argv[1], argv[2])).c_str());
    else if(argc == 4)  //3 arguments (init filename, cmdline, currentdir)
        DbgCmdExec(StringUtils::Utf16ToUtf8(StringUtils::sprintf(L"init \"%s\", \"%s\", \"%s\"", argv[1], argv[2], argv[3])).c_str());
    else if(argc == 5 && !_wcsicmp(argv[1], L"-a") && !_wcsicmp(argv[3], L"-e"))  //4 arguments (JIT)
        DbgCmdExec(StringUtils::Utf16ToUtf8(StringUtils::sprintf(L"attach .%s, .%s", argv[2], argv[4])).c_str()); //attach pid, event
    LocalFree(argv);

    dputs(QT_TRANSLATE_NOOP("DBG", "Initialization successful!"));
    bIsStopped = false;
    return nullptr;
}

/**
@brief This function is called when the user closes the debugger.
*/
extern "C" DLL_EXPORT void _dbg_dbgexitsignal()
{
    dputs(QT_TRANSLATE_NOOP("DBG", "Stopping running debuggee..."));
    cbDebugStop(0, 0);
    dputs(QT_TRANSLATE_NOOP("DBG", "Waiting for the debuggee to be stopped..."));
    if(!waitfor(WAITID_STOP, 10000)) //after this, debugging stopped
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "The debuggee does not close after 10 seconds. Probably the debugger state has been corrupted."));
    }
    dputs(QT_TRANSLATE_NOOP("DBG", "Aborting scripts..."));
    scriptabort();
    dputs(QT_TRANSLATE_NOOP("DBG", "Unloading plugins..."));
    pluginunloadall();
    dputs(QT_TRANSLATE_NOOP("DBG", "Stopping command thread..."));
    bStopCommandLoopThread = true;
    MsgFreeStack(gMsgStack);
    WaitForThreadTermination(hCommandLoopThread);
    dputs(QT_TRANSLATE_NOOP("DBG", "Cleaning up allocated data..."));
    cmdfree();
    varfree();
    yr_finalize();
    Capstone::GlobalFinalize();
    dputs(QT_TRANSLATE_NOOP("DBG", "Checking for mem leaks..."));
    if(auto memleakcount = memleaks())
        dprintf(QT_TRANSLATE_NOOP("DBG", "%d memory leak(s) found!\n"), memleakcount);
    else
        DeleteFileW(StringUtils::Utf8ToUtf16(alloctrace).c_str());
    dputs(QT_TRANSLATE_NOOP("DBG", "Cleaning up wait objects..."));
    waitdeinitialize();
    dputs(QT_TRANSLATE_NOOP("DBG", "Cleaning up debugger threads..."));
    dbgstop();
    dputs(QT_TRANSLATE_NOOP("DBG", "Saving notes..."));
    char* text = nullptr;
    GuiGetGlobalNotes(&text);
    if(text)
    {
        FileHelper::WriteAllText(notesFile, String(text));
        BridgeFree(text);
    }
    else
        DeleteFileW(StringUtils::Utf8ToUtf16(notesFile).c_str());
    dputs(QT_TRANSLATE_NOOP("DBG", "Exit signal processed successfully!"));
    bIsStopped = true;
}

extern "C" DLL_EXPORT bool _dbg_dbgcmddirectexec(const char* cmd)
{
    if(cmddirectexec(cmd) == false)
        return false;
    return true;
}

bool dbgisstopped()
{
    return bIsStopped;
}
