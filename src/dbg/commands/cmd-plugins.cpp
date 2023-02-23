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
    auto cip = GetContextDataEx(hActiveThread, UE_CIP);
    auto cipModBase = ModBaseFromAddr(cip);
    ScyllaStartGui(fdProcessInfo->dwProcessId, (HINSTANCE)cipModBase, cip);
    FreeLibrary(hScylla);
    bScyllaLoaded = false;
    return 0;
}

bool cbDebugStartScylla(int argc, char* argv[])
{
    if(bScyllaLoaded)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Scylla is already loaded"));
        return false;
    }
    bScyllaLoaded = true;
    CloseHandle(CreateThread(0, 0, scyllaThread, 0, 0, 0));
    return true;
}

bool cbInstrPluginLoad(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    return pluginload(argv[1]);
}

bool cbInstrPluginUnload(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    return pluginunload(argv[1]);
}

bool cbInstrPluginReload(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    if(!pluginunload(argv[1]))
        return false;
    if(argc <= 2)
    {
        auto text = StringUtils::Utf8ToUtf16(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Press OK to reload the plugin...")));
        auto title = StringUtils::Utf8ToUtf16(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Reload")));
        if(MessageBoxW(GuiGetWindowHandle(), text.c_str(), title.c_str(), MB_OKCANCEL) != IDOK)
            return true;
    }
    return pluginload(argv[1]);
}