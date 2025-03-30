#include "../bridge/bridgemain.h"
#include "../dbg/_plugins.h"

#include <string>
#include <iostream>
#include <vector>

#include "stringutils.h"

#define Cmd(x) DbgCmdExecDirect(x)
#define Eval(x) DbgValFromString(x)
#define dprintf(x, ...) _plugin_logprintf("[" PLUGIN_NAME "] " x, __VA_ARGS__)
#define dputs(x) _plugin_logprintf("[" PLUGIN_NAME "] %s\n", x)

static std::vector<SCRIPTTYPEINFO> scriptInfo;
static int curScriptId = 0;
static bool dbgStopped = false;

struct GuiState
{
    duint disasm = 0;
    duint cip = 0;
    duint dump = 0;
    duint stack = 0;
    duint csp = 0;
    duint graph = 0;
    duint memmap = 0;
    duint symmod = 0;
    std::string globalNotes;
    std::string debuggeeNotes;
} guistate;

extern "C" __declspec(dllexport) int _gui_guiinit(int argc, char* argv[])
{
    // Allocate console
    if(AllocConsole())
    {
        FILE* ptrNewStdIn = nullptr;
        FILE* ptrNewStdOut = nullptr;
        FILE* ptrNewStdErr = nullptr;

        freopen_s(&ptrNewStdIn, "CONIN$", "r", stdin);
        freopen_s(&ptrNewStdOut, "CONOUT$", "w", stdout);
        freopen_s(&ptrNewStdErr, "CONOUT$", "w", stderr);
    }

    // Init debugger
    const char* errormsg = DbgInit();
    if(errormsg)
    {
        MessageBoxA(0, errormsg, "DbgInit Error!", MB_SYSTEMMODAL | MB_ICONERROR);
        return 1;
    }
    while(true)
    {
        std::string command;
        std::getline(std::cin, command);
        if(command == "exit")
        {
            DbgExit();
            dbgStopped = true;
            break;
        }
        else if(command == "langs")
        {
            for(auto & info : scriptInfo)
                printf("%d:%s\n", info.id, info.name);
        }
        else if(command == "state")
        {
            printf("disasm: 0x%p\n", (void*)guistate.disasm);
            printf("   cip: 0x%p\n", (void*)guistate.cip);
            printf("  dump: 0x%p\n", (void*)guistate.dump);
            printf(" stack: 0x%p\n", (void*)guistate.stack);
            printf("   csp: 0x%p\n", (void*)guistate.csp);
            if(guistate.graph)
                printf("  graph: 0x%p\n", (void*)guistate.graph);
            if(guistate.memmap)
                printf(" memmap: 0x%p\n", (void*)guistate.memmap);
            if(guistate.symmod)
                printf("symmod: 0x%p\n", (void*)guistate.symmod);
        }
        else
        {
            int scriptId = 0;
            if(command.size() > 2 && isdigit(command[0]) && command[1] == '>')
            {
                scriptId = command[0] - '0';
                command = command.substr(2);
            }
            if(scriptId >= scriptInfo.size())
            {
                printf("[FAIL] no script id registered %d\n", scriptId);
                continue;
            }
            if(!scriptInfo[scriptId].execute(command.c_str()))
            {
                puts("[FAIL] command failed");
            }
        }
    }
    return 0;
}

#include "tostring.h"

extern "C" __declspec(dllexport) void* _gui_sendmessage(GUIMSG type, void* param1, void* param2)
{
    if(dbgStopped) //there can be no more messages if the debugger stopped = IGNORE
    {
        printf("[WARN] Ignored %s (%d)\n", guimsg2str(type), type);
        return nullptr;
    }

    switch(type)
    {
    case GUI_AUTOCOMPLETE_ADDCMD:
    case GUI_UPDATE_TIME_WASTED_COUNTER:
    case GUI_FLUSH_LOG:
    case GUI_INVALIDATE_SYMBOL_SOURCE:
    case GUI_UPDATE_ARGUMENT_VIEW:
    case GUI_UPDATE_BREAKPOINTS_VIEW:
    case GUI_UPDATE_CALLSTACK:
    case GUI_UPDATE_DISASSEMBLY_VIEW:
    case GUI_UPDATE_DUMP_VIEW:
    case GUI_UPDATE_GRAPH_VIEW:
    case GUI_UPDATE_MEMORY_VIEW:
    case GUI_UPDATE_PATCHES:
    case GUI_UPDATE_REGISTER_VIEW:
    case GUI_UPDATE_SEHCHAIN:
    case GUI_UPDATE_SIDEBAR:
    case GUI_UPDATE_THREAD_VIEW:
    case GUI_UPDATE_TRACE_BROWSER:
    case GUI_UPDATE_TYPE_WIDGET:
    case GUI_UPDATE_WATCH_VIEW:
    case GUI_SYMBOL_UPDATE_MODULE_LIST:
    case GUI_FOCUS_VIEW:
    case GUI_REPAINT_TABLE_VIEW:
    case GUI_ADD_RECENT_FILE:
    case GUI_SCRIPT_SETIP:
    case GUI_SHOW_CPU:
        break;

    case GUI_UPDATE_WINDOW_TITLE:
        SetConsoleTitleW(Utf8ToUtf16((const char*)param1).c_str());
        break;

    case GUI_GET_WINDOW_HANDLE:
        return GetConsoleWindow();

    case GUI_SYMBOL_LOG_ADD:
        printf("[SYMBOL] %s", (const char*)param1);
        break;

    case GUI_ADD_MSG_TO_LOG:
        printf("%s", (const char*)param1);
        break;

    case GUI_SET_DEBUG_STATE:
    {
        auto s = DBGSTATE(duint(param1));
        printf("[STATE] %s\n", dbgstate2str(s));
    }
    break;

    case GUI_REGISTER_SCRIPT_LANG:
    {
        SCRIPTTYPEINFO* info = (SCRIPTTYPEINFO*)param1;
        info->id = (int)scriptInfo.size();
        scriptInfo.push_back(*info);
    }
    break;

    case GUI_UNREGISTER_SCRIPT_LANG:
    {
        int id = (int)(duint)param1;
        if(id != 0)
        {
            puts("[TODO] Not implemented GUI_UNREGISTER_SCRIPT_LANG");
        }
    }
    break;

    case GUI_DUMP_AT:
        guistate.dump = (duint)param1;
        break;

    case GUI_DISASSEMBLE_AT:
        guistate.disasm = (duint)param1;
        guistate.cip = (duint)param2;
        break;

    case GUI_STACK_DUMP_AT:
        guistate.stack = (duint)param1;
        guistate.csp = (duint)param2;
        break;

    case GUI_SET_GLOBAL_NOTES:
    {
        if(param1)
            guistate.globalNotes = (const char*)param1;
    }
    break;

    case GUI_SET_DEBUGGEE_NOTES:
    {
        if(param1)
            guistate.debuggeeNotes = (const char*)param1;
    }
    break;

    case GUI_GET_GLOBAL_NOTES:
    {
        char* result = nullptr;
        if(!guistate.globalNotes.empty())
        {
            result = (char*)BridgeAlloc(guistate.globalNotes.size() + 1);
            strcpy_s(result, guistate.globalNotes.size() + 1, guistate.globalNotes.c_str());
        }
        *(char**)param1 = result;
    }
    break;

    case GUI_GET_DEBUGGEE_NOTES:
    {
        char* result = nullptr;
        if(!guistate.debuggeeNotes.empty())
        {
            result = (char*)BridgeAlloc(guistate.debuggeeNotes.size() + 1);
            strcpy_s(result, guistate.debuggeeNotes.size() + 1, guistate.debuggeeNotes.c_str());
        }
        *(char**)param1 = result;
    }
    break;

    case GUI_SELECTION_GET:
    {
        int hWindow = (int)(duint)param1;
        SELECTIONDATA* selection = (SELECTIONDATA*)param2;
        if(!DbgIsDebugging())
            return (void*)false;
        duint p = 0;
        switch(hWindow)
        {
        case GUI_DISASSEMBLY:
            p = guistate.disasm;
            break;
        case GUI_DUMP:
            p = guistate.dump;
            break;
        case GUI_STACK:
            p = guistate.stack;
            break;
        case GUI_GRAPH:
            p = guistate.graph;
            break;
        case GUI_MEMMAP:
            p = guistate.memmap;
            break;
        case GUI_SYMMOD:
            p = guistate.symmod;
            break;
        default:
            return (void*)false;
        }
        selection->start = selection->end = p;
        return (void*)true;
    }
    break;

    case GUI_SELECTION_SET:
    {
        int hWindow = (int)(duint)param1;
        const SELECTIONDATA* selection = (const SELECTIONDATA*)param2;
        if(!DbgIsDebugging())
            return (void*)false;
        duint p = 0;
        switch(hWindow)
        {
        case GUI_DISASSEMBLY:
            guistate.disasm = selection->start;
            break;
        case GUI_DUMP:
            guistate.dump = selection->start;
            break;
        case GUI_STACK:
            guistate.stack = selection->start;
            break;
        case GUI_GRAPH:
            guistate.graph = selection->start;
            break;
        case GUI_MEMMAP:
            guistate.memmap = selection->start;
            break;
        case GUI_SYMMOD:
            guistate.symmod = selection->start;
            break;
        default:
            return (void*)false;
        }
        return (void*)true;
    }
    break;

    default:
    {
        printf("[TODO] Not implemented %s (%d)\n", guimsg2str(type), type);
        break;
    }
    }
    return nullptr;
}

extern "C" __declspec(dllexport) const char* _gui_translate_text(const char* source)
{
    return source;
}

int main(int argc, char* argv[])
{
    // Construct user directory from executable name
    auto hMainModule = GetModuleHandleW(nullptr);
    wchar_t szUserDirectory[MAX_PATH] = L"";
    GetModuleFileNameW(hMainModule, szUserDirectory, _countof(szUserDirectory));
    auto period = wcsrchr(szUserDirectory, L'.');
    if(period == nullptr)
    {
        puts("Error getting module directory!");
        return EXIT_FAILURE;
    }
    *period = L'\0';
    CreateDirectoryW(szUserDirectory, nullptr);

    // Initialize the bridge
    BRIDGE_CONFIG config = {};
    config.hGuiModule = hMainModule;
    config.szUserDirectory = szUserDirectory;
    const wchar_t* errormsg = BridgeInit(&config);
    if(errormsg != nullptr)
    {
        wprintf(L"BridgeInit failed: %s\n", errormsg);
        return EXIT_FAILURE;
    }

    // Start the debugger
    errormsg = BridgeStart();
    if(errormsg != nullptr)
    {
        wprintf(L"BridgeStart failed: %s\n", errormsg);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
