#ifndef _PLUGINS_H
#define _PLUGINS_H

#ifndef PLUG_IMPEXP
#ifdef BUILD_DBG
#define PLUG_IMPEXP __declspec(dllexport)
#else
#define PLUG_IMPEXP __declspec(dllimport)
#endif //BUILD_DBG
#endif //PLUG_IMPEXP

#include "_plugin_types.h"

//structures
struct PLUG_INITSTRUCT
{
    //provided by the debugger
    int pluginHandle;
    //provided by the pluginit function
    int sdkVersion;
    int pluginVersion;
    char pluginName[256];
};

//callback structures
struct PLUG_CB_INITDEBUG
{
    const char* szFileName;
};

struct PLUG_CB_STOPDEBUG
{
    void* reserved;
};

struct PLUG_CB_CREATEPROCESS
{
    CREATE_PROCESS_DEBUG_INFO* CreateProcessInfo;
    IMAGEHLP_MODULE64* modInfo;
    const char* DebugFileName;
};

//enums
enum CBTYPE
{
    CB_INITDEBUG, //PLUG_CB_INITDEBUG
    CB_STOPDEBUG, //PLUG_CB_STOPDEBUG
    CB_CREATEPROCESS, //PLUG_CB_CREATEPROCESS
    CB_EXITPROCESS,
    CB_CREATETHREAD,
    CB_EXITTHREAD,
    CB_SYSTEMBREAKPOINT,
    CB_LOADDLL,
    CB_UNLOADDLL,
    CB_OUTPUTDEBUGSTRING,
    CB_EXCEPTION,
    CB_BREAKPOINT,
    CB_PAUSEDEBUG,
    CB_RESUMEDEBUG,
    CB_STEPPED
};

//typedefs
typedef void (*CBPLUGIN)(CBTYPE cbType, void* callbackInfo);

//exports
#ifdef __cplusplus
extern "C"
{
#endif

PLUG_IMPEXP void _plugin_registercallback(int pluginHandle, CBTYPE cbType, CBPLUGIN cbPlugin);
PLUG_IMPEXP bool _plugin_unregistercallback(int pluginHandle, CBTYPE cbType);

#ifdef __cplusplus
}
#endif

#endif // _PLUGINS_H
