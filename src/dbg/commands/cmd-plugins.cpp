#include "cmd-plugins.h"
#include "console.h"
#include "debugger.h"
#include "module.h"
#include "plugin_loader.h"

static bool bScyllaLoaded = false;

static DWORD WINAPI scyllaThread(void* lpParam)
{
    typedef INT(WINAPI * SCYLLASTARTGUI)(DWORD pid, HINSTANCE mod, DWORD_PTR entrypoint);
    SCYLLASTARTGUI ScyllaStartGui = 0;
    HINSTANCE hScylla = LoadLibraryW(L"Scylla.dll");
    if(!hScylla)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Error loading Scylla.dll!"));
        bScyllaLoaded = false;
        return 0;
    }
    ScyllaStartGui = (SCYLLASTARTGUI)GetProcAddress(hScylla, "ScyllaStartGui");
    if(!ScyllaStartGui)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Could not find export 'ScyllaStartGui' inside Scylla.dll"));
        bScyllaLoaded = false;
        FreeLibrary(hScylla);
        return 0;
    }
    auto cip = GetContextDataEx(fdProcessInfo->hThread, UE_CIP);
    auto cipModBase = ModBaseFromAddr(cip);
    ScyllaStartGui(fdProcessInfo->dwProcessId, (HINSTANCE)cipModBase, cip);
    FreeLibrary(hScylla);
    bScyllaLoaded = false;
    return 0;
}

CMDRESULT cbDebugStartScylla(int argc, char* argv[])
{
    if(bScyllaLoaded)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Scylla is already loaded"));
        return STATUS_ERROR;
    }
    bScyllaLoaded = true;
    CloseHandle(CreateThread(0, 0, scyllaThread, 0, 0, 0));
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrPluginLoad(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return STATUS_ERROR;
    if(pluginload(argv[1]))
        return STATUS_CONTINUE;
    return STATUS_ERROR;
}

CMDRESULT cbInstrPluginUnload(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return STATUS_ERROR;
    if(pluginunload(argv[1]))
        return STATUS_CONTINUE;
    return STATUS_ERROR;
}
