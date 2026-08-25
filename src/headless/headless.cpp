#include "../bridge/bridgemain.h"
#include "../bridge/startupargs.h"
#include "../dbg/_plugins.h"
#include "../dbg/concurrentqueue/blockingconcurrentqueue.h"

#include <atomic>
#include <cstdio>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>

#include "stringutils.h"

#define Cmd(x) DbgCmdExecAsyncDirect(x)
#define Eval(x) DbgValFromString(x)
#define dprintf(x, ...) _plugin_logprintf("[" PLUGIN_NAME "] " x, __VA_ARGS__)
#define dputs(x) _plugin_logprintf("[" PLUGIN_NAME "] %s\n", x)

static std::vector<SCRIPTTYPEINFO> scriptInfo;
static int curScriptId = 0;
static bool dbgStopped = false;
static DWORD dwGuiThreadId = 0;
static moodycamel::BlockingConcurrentQueue<std::function<bool()>> queue;
static constexpr DWORD NoShutdownCtrlType = MAXDWORD;
static std::atomic<bool> shutdownRequested{ false };
static std::atomic<bool> consoleCloseRequested{ false };
static std::atomic<DWORD> shutdownCtrlType{ NoShutdownCtrlType };
static std::atomic<HANDLE> commandThreadHandle{ nullptr };
static std::mutex redirectLogMutex;
static FILE* redirectLogFile = nullptr;

static void stopRedirectLog()
{
    std::lock_guard<std::mutex> lock(redirectLogMutex);
    if(redirectLogFile)
    {
        fclose(redirectLogFile);
        redirectLogFile = nullptr;
    }
}

static void startRedirectLog(const char* filename)
{
    stopRedirectLog();
    if(filename == nullptr || *filename == '\0')
        return;

    FILE* file = nullptr;
    if(_wfopen_s(&file, Utf8ToUtf16(filename).c_str(), L"ab") != 0 || file == nullptr)
    {
        printf("[headless] failed to redirect log to %s\n", filename);
        return;
    }

    std::lock_guard<std::mutex> lock(redirectLogMutex);
    redirectLogFile = file;
}

static void appendRedirectedLog(const char* text)
{
    if(text == nullptr || *text == '\0')
        return;

    std::lock_guard<std::mutex> lock(redirectLogMutex);
    if(redirectLogFile == nullptr)
        return;

    fwrite(text, 1, strlen(text), redirectLogFile);
    fflush(redirectLogFile);
}

static void requestShutdown()
{
    if(shutdownRequested.exchange(true))
        return;
    queue.enqueue([]()
    {
        return false;
    });
}

static const char* shutdownCtrlTypeToString(DWORD ctrlType)
{
    switch(ctrlType)
    {
    case CTRL_C_EVENT:
        return "Ctrl+C";
    case CTRL_BREAK_EVENT:
        return "Ctrl+Break";
    case CTRL_CLOSE_EVENT:
        return "console close";
    case CTRL_LOGOFF_EVENT:
        return "logoff";
    case CTRL_SHUTDOWN_EVENT:
        return "system shutdown";
    default:
        return nullptr;
    }
}

static BOOL WINAPI consoleCtrlHandler(DWORD ctrlType)
{
    switch(ctrlType)
    {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        shutdownCtrlType = ctrlType;
        consoleCloseRequested = true;
        requestShutdown();
        if(auto handle = commandThreadHandle.load())
            CancelSynchronousIo(handle);
        return TRUE;
    default:
        return FALSE;
    }
}

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
    // Disable buffering for stdout and stderr to ensure immediate output
    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);

    shutdownRequested = false;
    consoleCloseRequested = false;
    shutdownCtrlType = NoShutdownCtrlType;
    commandThreadHandle = nullptr;

    // Init debugger
    const char* errormsg = DbgInitBlocking();
    if(errormsg)
    {
        puts(errormsg);
        return 1;
    }
    SetConsoleCtrlHandler(consoleCtrlHandler, TRUE);

    puts("[headless] entering command loop...");
    std::thread commandThread([]()
    {
        while(!shutdownRequested.load())
        {
            std::string command;
            if(!std::getline(std::cin, command))
            {
                if(!DbgIsTesting())
                    requestShutdown();
                break;
            }
            if(command == "exit")
            {
                requestShutdown();
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
                queue.enqueue([scriptId, command]()
                {
                    if(!scriptInfo[scriptId].execute(command.c_str()))
                    {
                        puts("[FAIL] command failed");
                    }
                    return true;
                });
            }
        }
    });
    commandThreadHandle = (HANDLE)commandThread.native_handle();
    while(true)
    {
        std::function<bool()> job;
        queue.wait_dequeue(job);
        if(!job())
        {
            if(const auto reason = shutdownCtrlTypeToString(shutdownCtrlType.load()))
                printf("[headless] shutdown requested by %s\n", reason);
            DbgExit();
            dbgStopped = true;
            if(consoleCloseRequested.load())
            {
                if(auto handle = commandThreadHandle.load())
                    CancelSynchronousIo(handle);
            }
            break;
        }
    }
    if(commandThread.joinable())
        commandThread.join();
    commandThreadHandle = nullptr;
    stopRedirectLog();
    SetConsoleCtrlHandler(consoleCtrlHandler, FALSE);
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

    case GUI_CLOSE_APPLICATION:
        requestShutdown();
        if(auto handle = commandThreadHandle.load())
            CancelSynchronousIo(handle);
        break;

    case GUI_SYMBOL_LOG_ADD:
        printf("[SYMBOL] %s", (const char*)param1);
        if(param1)
        {
            std::string line = std::string("[SYMBOL] ") + (const char*)param1;
            appendRedirectedLog(line.c_str());
        }
        break;

    case GUI_ADD_MSG_TO_LOG_HTML:
    case GUI_ADD_MSG_TO_LOG:
        printf("%s", (const char*)param1);
        appendRedirectedLog((const char*)param1);
        break;

    case GUI_REDIRECT_LOG:
        startRedirectLog((const char*)param1);
        break;

    case GUI_STOP_REDIRECT_LOG:
        stopRedirectLog();
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

    case GUI_EXECUTE_ON_GUI_THREAD:
    {
        if(GetCurrentThreadId() == dwGuiThreadId)
            ((GUICALLBACKEX)param1)(param2);
        else
        {
            queue.enqueue([param1, param2]()
            {
                ((GUICALLBACKEX)param1)(param2);
                return true;
            });
        }
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
    // GitHub Actions and other Node-based parents can set
    // SEM_NOGPFAULTERRORBOX. Clear it so WER LocalDumps can capture an
    // unhandled headless crash without showing legacy hard-error dialogs.
    SetErrorMode(SEM_FAILCRITICALERRORS);

    dwGuiThreadId = GetCurrentThreadId();

    // Construct user directory from executable name
    auto hMainModule = GetModuleHandleW(nullptr);
    const auto startupOptions = ParseHostStartupOptions();

    std::wstring userDirectory;
    if(!startupOptions.userDirectory.empty())
    {
        userDirectory = startupOptions.userDirectory;
    }
    else
    {
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
        userDirectory = szUserDirectory;
    }

    // Initialize the bridge
    BRIDGE_CONFIG config = {};
    config.hGuiModule = hMainModule;
    config.szUserDirectory = userDirectory.c_str();
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
