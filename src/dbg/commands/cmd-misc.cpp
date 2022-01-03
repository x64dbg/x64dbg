#include "cmd-misc.h"
#include "exprfunc.h"
#include "variable.h"
#include "value.h"
#include "debugger.h"
#include "threading.h"
#include "thread.h"
#include "assemble.h"
#include "memory.h"
#include "plugin_loader.h"
#include "jit.h"
#include "mnemonichelp.h"
#include "commandline.h"
#include "stringformat.h"

bool cbInstrChd(int argc, char* argv[])
{
    String directory;
    if(argc < 2)
    {
        directory = szProgramDir;
    }
    else
    {
        directory = argv[1];
    }
    if(!DirExists(argv[1]))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Directory '%s' doesn't exist\n"), directory.c_str());
        return false;
    }
    SetCurrentDirectoryW(StringUtils::Utf8ToUtf16(directory).c_str());
    dputs(QT_TRANSLATE_NOOP("DBG", "Current directory changed!"));
    return true;
}

bool cbInstrZzz(int argc, char* argv[])
{
    duint value = 100;
    if(argc > 1)
        if(!valfromstring(argv[1], &value, false))
            return false;
    auto ms = DWORD(value);
    if(ms == INFINITE)
        ms = 100;
    Sleep(ms);
    return true;
}

bool cbDebugHide(int argc, char* argv[])
{
    if(HideDebugger(fdProcessInfo->hProcess, UE_HIDE_PEBONLY))
        dputs(QT_TRANSLATE_NOOP("DBG", "Debugger hidden"));
    else
        dputs(QT_TRANSLATE_NOOP("DBG", "Something went wrong"));
    return true;
}

static duint LoadLibThreadID;
static duint FreeLibThreadID;
static duint DLLNameMem;
static duint ASMAddr;
static TITAN_ENGINE_CONTEXT_t backupctx = { 0 };

static void cbDebugLoadLibBPX()
{
    HANDLE LoadLibThread = ThreadGetHandle((DWORD)LoadLibThreadID);
#ifdef _WIN64
    duint LibAddr = GetContextDataEx(LoadLibThread, UE_RAX);
#else
    duint LibAddr = GetContextDataEx(LoadLibThread, UE_EAX);
#endif //_WIN64
    varset("$result", LibAddr, false);
    backupctx.eflags &= ~0x100;
    SetFullContextDataEx(LoadLibThread, &backupctx);
    MemFreeRemote(DLLNameMem);
    MemFreeRemote(ASMAddr);
    ThreadResumeAll();
    //update GUI
    DebugUpdateGuiSetStateAsync(GetContextDataEx(hActiveThread, UE_CIP), true);
    //lock
    lock(WAITID_RUN);
    dbgsetforeground();
    PLUG_CB_PAUSEDEBUG pauseInfo = { nullptr };
    plugincbcall(CB_PAUSEDEBUG, &pauseInfo);
    wait(WAITID_RUN);
}

bool cbDebugLoadLib(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Error: you must specify the name of the DLL to load\n"));
        return false;
    }

#ifdef _WIN64
    unsigned char loader[] =
    {
        0x48, 0xB9, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, //movabs rcx, DLLNameAddr
        0x48, 0xB8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, //movabs rax, p_LoadLibraryW
        0xFF, 0xD0, //call rax
        0x90 //nop
    };
#else
    unsigned char loader[] =
    {
        0x68, 0xFF, 0xFF, 0xFF, 0xFF, //push DLLNameMem
        0xB8, 0xFF, 0xFF, 0xFF, 0xFF, //mov eax, p_LoadLibraryW
        0xFF, 0xD0, //call eax
        0x90 //nop
    };
#endif //_WIN64
    auto DLLNameOffset = ArchValue(1, 2), LoadLibraryOffset = ArchValue(6, 12);

    LoadLibThreadID = fdProcessInfo->dwThreadId;
    HANDLE LoadLibThread = ThreadGetHandle((DWORD)LoadLibThreadID);

    auto DLLNameW = StringUtils::Utf8ToUtf16(argv[1]);
    auto DLLNameSize = (DLLNameW.length() + 1) * 2;

    duint p_LoadLibraryW = 0;
    if(!valfromstring("kernel32:LoadLibraryW", &p_LoadLibraryW, false))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Error: couldn't get kernel32:LoadLibraryW"));
        return false;
    }

    ASMAddr = MemAllocRemote(0, sizeof(loader));
    DLLNameMem = MemAllocRemote(0, DLLNameSize);
    if(!ASMAddr || !DLLNameMem)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Error: couldn't allocate memory in debuggee"));
        return false;
    }

    // Set addresses in the loader
    memcpy(loader + DLLNameOffset, &DLLNameMem, sizeof(duint));
    memcpy(loader + LoadLibraryOffset, &p_LoadLibraryW, sizeof(duint));

    if(!MemWrite(ASMAddr, loader, sizeof(loader)) || !MemWrite(DLLNameMem, DLLNameW.c_str(), DLLNameSize))
    {
        MemFreeRemote(ASMAddr);
        dputs(QT_TRANSLATE_NOOP("DBG", "Error: couldn't write process memory"));
        return false;
    }

    if(!SetBPX(ASMAddr + sizeof(loader) - 1, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, (void*)cbDebugLoadLibBPX))
    {
        MemFreeRemote(ASMAddr);
        dputs(QT_TRANSLATE_NOOP("DBG", "Error: couldn't SetBPX"));
        return false;
    }

    ThreadSuspendAll();
    GetFullContextDataEx(LoadLibThread, &backupctx);
    SetContextDataEx(LoadLibThread, UE_CIP, ASMAddr);
    SetContextDataEx(LoadLibThread, UE_CSP, backupctx.csp & ~0xF);
    ResumeThread(LoadLibThread);

    unlock(WAITID_RUN);

    return true;
}

static void cbDebugFreeLibBPX()
{
    HANDLE FreeLibThread = ThreadGetHandle((DWORD)FreeLibThreadID);
#ifdef _WIN64
    duint LibAddr = GetContextDataEx(FreeLibThread, UE_RAX);
#else
    duint LibAddr = GetContextDataEx(FreeLibThread, UE_EAX);
#endif //_WIN64
    varset("$result", LibAddr, false);
    backupctx.eflags &= ~0x100;
    SetFullContextDataEx(FreeLibThread, &backupctx);
    MemFreeRemote(ASMAddr);
    ThreadResumeAll();
    //update GUI
    DebugUpdateGuiSetStateAsync(GetContextDataEx(hActiveThread, UE_CIP), true);
    //lock
    lock(WAITID_RUN);
    dbgsetforeground();
    PLUG_CB_PAUSEDEBUG pauseInfo = { nullptr };
    plugincbcall(CB_PAUSEDEBUG, &pauseInfo);
    wait(WAITID_RUN);
}

bool cbDebugFreeLib(int argc, char* argv[])
{
    duint base = 0;
    if(IsArgumentsLessThan(argc, 2) || !valfromstring(argv[1], &base, false))
        return false;
    base = ModBaseFromAddr(base);
    if(!base)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Error: the specified address does not point inside a module"));
        return false;
    }

    unsigned char loader[] =
#ifdef _WIN64
    {
        0x48, 0xB9, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, //movabs rcx, ModuleBase
        0x48, 0xB8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, //movabs rax, p_FreeLibrary
        0xFF, 0xD0, //call rax
        0x90 //nop
    };
#else
        {
            0x68, 0xFF, 0xFF, 0xFF, 0xFF, //push ModuleBase
            0xB8, 0xFF, 0xFF, 0xFF, 0xFF, //mov eax, p_FreeLibrary
            0xFF, 0xD0, //call eax
            0x90 //nop
        };
#endif //_WIN64
    auto ModuleBaseOffset = ArchValue(1, 2), FreeLibraryOffset = ArchValue(6, 12);

    FreeLibThreadID = fdProcessInfo->dwThreadId;
    HANDLE UnLoadLibThread = ThreadGetHandle((DWORD)FreeLibThreadID);

    duint p_FreeLibrary = 0;
    if(!valfromstring("kernel32:FreeLibrary", &p_FreeLibrary, false))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Error: couldn't get kernel32:FreeLibrary"));
        return false;
    }

    ASMAddr = MemAllocRemote(0, sizeof(loader));
    if(!ASMAddr)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Error: couldn't allocate memory in debuggee"));
        return false;
    }

    // Set addresses in the loader
    memcpy(loader + ModuleBaseOffset, &base, sizeof(duint));
    memcpy(loader + FreeLibraryOffset, &p_FreeLibrary, sizeof(duint));

    if(!MemWrite(ASMAddr, loader, sizeof(loader)))
    {
        MemFreeRemote(ASMAddr);
        dputs(QT_TRANSLATE_NOOP("DBG", "Error: couldn't write process memory"));
        return false;
    }

    if(!SetBPX(ASMAddr + sizeof(loader) - 1, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, (void*)cbDebugFreeLibBPX))
    {
        MemFreeRemote(ASMAddr);
        dputs(QT_TRANSLATE_NOOP("DBG", "Error: couldn't SetBPX"));
        return false;
    }

    ThreadSuspendAll();
    GetFullContextDataEx(UnLoadLibThread, &backupctx);
    SetContextDataEx(UnLoadLibThread, UE_CIP, ASMAddr);
    ResumeThread(UnLoadLibThread);
    unlock(WAITID_RUN);
    return true;
}

bool cbInstrAssemble(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return false;
    duint addr = 0;
    if(!valfromstring(argv[1], &addr))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid expression: \"%s\"!\n"), argv[1]);
        return false;
    }
    if(!DbgMemIsValidReadPtr(addr))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid address: %p!\n"), addr);
        return false;
    }
    bool fillnop = false;
    if(argc > 3)
        fillnop = true;
    char error[MAX_ERROR_SIZE] = "";
    int size = 0;
    auto asmFormat = stringformatinline(argv[2]);
    if(!assembleat(addr, asmFormat.c_str(), &size, error, fillnop))
    {
        varset("$result", size, false);
        dprintf(QT_TRANSLATE_NOOP("DBG", "Failed to assemble \"%s\" (%s)\n"), asmFormat.c_str(), error);
        return false;
    }
    varset("$result", size, false);
    GuiUpdateAllViews();
    return true;
}

bool cbInstrGpa(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    char newcmd[deflen] = "";
    if(argc >= 3)
        sprintf_s(newcmd, "\"%s\":%s", argv[2], argv[1]);
    else
        sprintf_s(newcmd, "%s", argv[1]);
    duint result = 0;
    if(!valfromstring(newcmd, &result, false))
        return false;
    varset("$RESULT", result, false);
    return true;
}

bool cbDebugSetJIT(int argc, char* argv[])
{
    arch actual_arch = notfound;
    char* jit_debugger_cmd = "";
    Memory<char*> oldjit(MAX_SETTING_SIZE + 1);
    char path[JIT_ENTRY_DEF_SIZE];
    if(!BridgeIsProcessElevated())
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Error run the debugger as Admin to setjit\n"));
        return false;
    }
    if(argc < 2)
    {
        dbggetdefjit(path);

        jit_debugger_cmd = path;
        if(!dbgsetjit(jit_debugger_cmd, notfound, &actual_arch, NULL))
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "Error setting JIT %s\n"), (actual_arch == x64) ? "x64" : "x32");
            return false;
        }
    }
    else if(argc == 2)
    {
        if(!_strcmpi(argv[1], "old"))
        {
            jit_debugger_cmd = oldjit();
            if(!BridgeSettingGet("JIT", "Old", jit_debugger_cmd))
            {
                dputs(QT_TRANSLATE_NOOP("DBG", "Error there is no old JIT entry stored."));
                return false;
            }

            if(!dbgsetjit(jit_debugger_cmd, notfound, &actual_arch, NULL))
            {
                dprintf(QT_TRANSLATE_NOOP("DBG", "Error setting JIT %s\n"), (actual_arch == x64) ? "x64" : "x32");
                return false;
            }
        }
        else if(!_strcmpi(argv[1], "oldsave"))
        {
            dbggetdefjit(path);
            char get_entry[JIT_ENTRY_MAX_SIZE] = "";
            bool get_last_jit = true;

            if(!dbggetjit(get_entry, notfound, &actual_arch, NULL))
            {
                get_last_jit = false;
            }
            else
                strcpy_s(oldjit(), MAX_SETTING_SIZE, get_entry);

            jit_debugger_cmd = path;
            if(!dbgsetjit(jit_debugger_cmd, notfound, &actual_arch, NULL))
            {
                dprintf(QT_TRANSLATE_NOOP("DBG", "Error setting JIT %s\n"), (actual_arch == x64) ? "x64" : "x32");
                return false;
            }
            if(get_last_jit)
            {
                if(_stricmp(oldjit(), path))
                    BridgeSettingSet("JIT", "Old", oldjit());
            }
        }
        else if(!_strcmpi(argv[1], "restore"))
        {
            jit_debugger_cmd = oldjit();

            if(!BridgeSettingGet("JIT", "Old", jit_debugger_cmd))
            {
                dputs(QT_TRANSLATE_NOOP("DBG", "Error there is no old JIT entry stored."));
                return false;
            }

            if(!dbgsetjit(jit_debugger_cmd, notfound, &actual_arch, NULL))
            {
                dprintf(QT_TRANSLATE_NOOP("DBG", "Error setting JIT %s\n"), (actual_arch == x64) ? "x64" : "x32");
                return false;
            }
            BridgeSettingSet("JIT", 0, 0);
        }
        else
        {
            jit_debugger_cmd = argv[1];
            if(!dbgsetjit(jit_debugger_cmd, notfound, &actual_arch, NULL))
            {
                dprintf(QT_TRANSLATE_NOOP("DBG", "Error setting JIT %s\n"), (actual_arch == x64) ? "x64" : "x32");
                return false;
            }
        }
    }
    else if(argc == 3)
    {
        readwritejitkey_error_t rw_error;

        if(!_strcmpi(argv[1], "old"))
        {
            BridgeSettingSet("JIT", "Old", argv[2]);

            dprintf(QT_TRANSLATE_NOOP("DBG", "New OLD JIT stored: %s\n"), argv[2]);

            return true;
        }

        else if(_strcmpi(argv[1], "x64") == 0)
            actual_arch = x64;
        else if(_strcmpi(argv[1], "x32") == 0)
            actual_arch = x32;
        else
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "Unknown JIT entry type. Use OLD, x64 or x32 as parameter."));
            return false;
        }

        jit_debugger_cmd = argv[2];
        if(!dbgsetjit(jit_debugger_cmd, actual_arch, NULL, &rw_error))
        {
            if(rw_error == ERROR_RW_NOTWOW64)
                dputs(QT_TRANSLATE_NOOP("DBG", "Error using x64 arg. The debugger is not a WOW64 process\n"));
            else
                dprintf(QT_TRANSLATE_NOOP("DBG", "Error setting JIT %s\n"), (actual_arch == x64) ? "x64" : "x32");
            return false;
        }
    }
    else
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Error unknown parameters. Use old, oldsave, restore, x86 or x64 as parameter."));
        return false;
    }

    dprintf(QT_TRANSLATE_NOOP("DBG", "New JIT %s: %s\n"), (actual_arch == x64) ? "x64" : "x32", jit_debugger_cmd);

    return true;
}

bool cbDebugGetJIT(int argc, char* argv[])
{
    char get_entry[JIT_ENTRY_MAX_SIZE] = "";
    arch actual_arch;

    if(argc < 2)
    {
        if(!dbggetjit(get_entry, notfound, &actual_arch, NULL))
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "Error getting JIT %s\n"), (actual_arch == x64) ? "x64" : "x32");
            return false;
        }
    }
    else
    {
        readwritejitkey_error_t rw_error;
        Memory<char*> oldjit(MAX_SETTING_SIZE + 1);
        if(_strcmpi(argv[1], "OLD") == 0)
        {
            if(!BridgeSettingGet("JIT", "Old", oldjit()))
            {
                dputs(QT_TRANSLATE_NOOP("DBG", "Error there is no old JIT entry stored."));
                return false;
            }
            else
            {
                dprintf(QT_TRANSLATE_NOOP("DBG", "OLD JIT entry stored: %s\n"), oldjit());
                return true;
            }
        }
        else if(_strcmpi(argv[1], "x64") == 0)
            actual_arch = x64;
        else if(_strcmpi(argv[1], "x32") == 0)
            actual_arch = x32;
        else
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "Unknown JIT entry type. Use OLD, x64 or x32 as parameter."));
            return false;
        }

        if(!dbggetjit(get_entry, actual_arch, NULL, &rw_error))
        {
            if(rw_error == ERROR_RW_NOTWOW64)
                dputs(QT_TRANSLATE_NOOP("DBG", "Error using x64 arg. The debugger is not a WOW64 process\n"));
            else
                dprintf(QT_TRANSLATE_NOOP("DBG", "Error getting JIT %s\n"), argv[1]);
            return false;
        }
    }

    dprintf(QT_TRANSLATE_NOOP("DBG", "JIT %s: %s\n"), (actual_arch == x64) ? "x64" : "x32", get_entry);

    return true;
}

bool cbDebugGetJITAuto(int argc, char* argv[])
{
    bool jit_auto = false;
    arch actual_arch = notfound;

    if(argc == 1)
    {
        if(!dbggetjitauto(&jit_auto, notfound, &actual_arch, NULL))
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "Error getting JIT auto %s\n"), (actual_arch == x64) ? "x64" : "x32");
            return false;
        }
    }
    else if(argc == 2)
    {
        readwritejitkey_error_t rw_error;
        if(_strcmpi(argv[1], "x64") == 0)
            actual_arch = x64;
        else if(_strcmpi(argv[1], "x32") == 0)
            actual_arch = x32;
        else
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "Unknown JIT auto entry type. Use x64 or x32 as parameter."));
            return false;
        }

        if(!dbggetjitauto(&jit_auto, actual_arch, NULL, &rw_error))
        {
            if(rw_error == ERROR_RW_NOTWOW64)
                dputs(QT_TRANSLATE_NOOP("DBG", "Error using x64 arg the debugger is not a WOW64 process\n"));
            else
                dprintf(QT_TRANSLATE_NOOP("DBG", "Error getting JIT auto %s\n"), argv[1]);
            return false;
        }
    }
    else
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Unknown JIT auto entry type. Use x64 or x32 as parameter."));
    }

    dprintf(QT_TRANSLATE_NOOP("DBG", "JIT auto %s: %s\n"), (actual_arch == x64) ? "x64" : "x32", jit_auto ? "ON" : "OFF");

    return true;
}

bool cbDebugSetJITAuto(int argc, char* argv[])
{
    arch actual_arch;
    bool set_jit_auto;
    if(!BridgeIsProcessElevated())
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Error run the debugger as Admin to setjitauto\n"));
        return false;
    }
    if(argc < 2)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Error setting JIT Auto. Use ON:1 or OFF:0 arg or x64/x32, ON:1 or OFF:0.\n"));
        return false;
    }
    else if(argc == 2)
    {
        if(_strcmpi(argv[1], "1") == 0 || _strcmpi(argv[1], "ON") == 0)
            set_jit_auto = true;
        else if(_strcmpi(argv[1], "0") == 0 || _strcmpi(argv[1], "OFF") == 0)
            set_jit_auto = false;
        else
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "Error unknown parameters. Use ON:1 or OFF:0"));
            return false;
        }

        if(!dbgsetjitauto(set_jit_auto, notfound, &actual_arch, NULL))
        {
            if(actual_arch == x64)
                dputs(QT_TRANSLATE_NOOP("DBG", "Error setting JIT auto x64"));
            else
                dputs(QT_TRANSLATE_NOOP("DBG", "Error setting JIT auto x32"));
            return false;
        }
    }
    else if(argc == 3)
    {
        readwritejitkey_error_t rw_error;
        actual_arch = x64;

        if(_strcmpi(argv[1], "x64") == 0)
            actual_arch = x64;
        else if(_strcmpi(argv[1], "x32") == 0)
            actual_arch = x32;
        else
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "Unknown JIT auto entry type. Use x64 or x32 as parameter."));
            return false;
        }

        if(_strcmpi(argv[2], "1") == 0 || _strcmpi(argv[2], "ON") == 0)
            set_jit_auto = true;
        else if(_strcmpi(argv[2], "0") == 0 || _strcmpi(argv[2], "OFF") == 0)
            set_jit_auto = false;
        else
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "Error unknown parameters. Use x86 or x64 and ON:1 or OFF:0\n"));
            return false;
        }

        if(!dbgsetjitauto(set_jit_auto, actual_arch, NULL, &rw_error))
        {
            if(rw_error == ERROR_RW_NOTWOW64)
                dputs(QT_TRANSLATE_NOOP("DBG", "Error using x64 arg the debugger is not a WOW64 process\n"));
            else
            {
                if(actual_arch == x64)
                    dputs(QT_TRANSLATE_NOOP("DBG", "Error getting JIT auto x64"));
                else
                    dputs(QT_TRANSLATE_NOOP("DBG", "Error getting JIT auto x32"));
            }
            return false;
        }
    }
    else
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Error unknown parameters use x86 or x64, ON/1 or OFF/0\n"));
        return false;
    }

    dprintf(QT_TRANSLATE_NOOP("DBG", "New JIT auto %s: %s\n"), (actual_arch == x64) ? "x64" : "x32", set_jit_auto ? "ON" : "OFF");
    return true;
}

bool cbDebugGetCmdline(int argc, char* argv[])
{
    char* cmd_line;
    cmdline_error_t cmdline_error = { (cmdline_error_type_t)0, 0 };

    if(!dbggetcmdline(&cmd_line, &cmdline_error))
    {
        showcommandlineerror(&cmdline_error);
        return false;
    }

    dprintf(QT_TRANSLATE_NOOP("DBG", "Command line: %s\n"), cmd_line);

    efree(cmd_line);

    return true;
}

bool cbDebugSetCmdline(int argc, char* argv[])
{
    cmdline_error_t cmdline_error = { (cmdline_error_type_t)0, 0 };

    if(argc != 2)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Error: write the arg1 with the new command line of the process debugged"));
        return false;
    }

    if(!dbgsetcmdline(argv[1], &cmdline_error))
    {
        showcommandlineerror(&cmdline_error);
        return false;
    }

    //update the memory map
    MemUpdateMap();
    GuiUpdateMemoryView();

    dprintf(QT_TRANSLATE_NOOP("DBG", "New command line: %s\n"), argv[1]);

    return true;
}

bool cbInstrMnemonichelp(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    auto description = MnemonicHelp::getDescription(argv[1]);
    if(!description.length())
        dputs(QT_TRANSLATE_NOOP("DBG", "No description or empty description"));
    else
    {
        auto padding = "================================================================";
        String logText = padding;
        logText += '\n';
        logText += description;
        logText += '\n';
        logText += padding;
        logText += '\n';
        GuiAddLogMessage(logText.c_str());
    }
    return true;
}

bool cbInstrMnemonicbrief(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    dputs(MnemonicHelp::getBriefDescription(argv[1]).c_str());
    return true;
}

bool cbInstrConfig(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return false;
    duint val = 0;
    if(argc == 3)
    {
        if(BridgeSettingGetUint(argv[1], argv[2], &val))
        {
            varset("$result", val, false);
            return true;
        }
        else
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "Error: Configuration not found."));
            return false;
        }
    }
    else
    {
        if(valfromstring(argv[3], &val, true))
        {
            if(BridgeSettingSetUint(argv[1], argv[2], val))
            {
                DbgSettingsUpdated();
                return true;
            }
            else
            {
                dputs(QT_TRANSLATE_NOOP("DBG", "Error updating configuration."));
                return false;
            }
        }
        else
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid expression: \"%s\"!\n"), argv[3]);
            return false;
        }
    }
}

bool cbInstrRestartadmin(int argc, char* argv[])
{
    if(dbgrestartadmin())
        GuiCloseApplication();
    return true;
}
