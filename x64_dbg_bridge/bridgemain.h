#ifndef _BRIDGEMAIN_H_
#define _BRIDGEMAIN_H_

#include <windows.h>

#ifdef _WIN64
typedef unsigned long long duint;
typedef signed long long dsint;
#else
typedef unsigned long duint;
typedef signed long dsint;
#endif //_WIN64

#ifndef BRIDGE_IMPEXP
#ifdef BUILD_BRIDGE
#define BRIDGE_IMPEXP __declspec(dllexport)
#else
#define BRIDGE_IMPEXP __declspec(dllimport)
#endif //BUILD_BRIDGE
#endif //BRIDGE_IMPEXP

#ifdef __cplusplus
extern "C"
{
#endif

//Bridge defines
#define MAX_SETTING_SIZE 2048

//Bridge functions
BRIDGE_IMPEXP const char* BridgeInit();
BRIDGE_IMPEXP const char* BridgeStart();
BRIDGE_IMPEXP void* BridgeAlloc(size_t size);
BRIDGE_IMPEXP void BridgeFree(void* ptr);
BRIDGE_IMPEXP bool BridgeSettingGet(const char* section, const char* key, char* value);
BRIDGE_IMPEXP bool BridgeSettingGetUint(const char* section, const char* key, duint* value);
BRIDGE_IMPEXP bool BridgeSettingSet(const char* section, const char* key, const char* value);
BRIDGE_IMPEXP bool BridgeSettingSetUint(const char* section, const char* key, duint value);

//Debugger defines
#define MAX_LABEL_SIZE 256
#define MAX_COMMENT_SIZE 256
#define MAX_MODULE_SIZE 256
#define MAX_BREAKPOINT_SIZE 256

//Debugger enums
enum DBGSTATE
{
    initialized,
    paused,
    running,
    stopped
};

enum SEGMENTREG
{
    SEG_DEFAULT,
    SEG_ES,
    SEG_DS,
    SEG_FS,
    SEG_GS,
    SEG_CS,
    SEG_SS
};

enum ADDRINFOFLAGS
{
    flagmodule=1,
    flaglabel=2,
    flagcomment=4,
    flagbookmark=8
};

enum BPXTYPE
{
    bp_none=0,
    bp_normal=1,
    bp_hardware=2,
    bp_memory=4
};

//Debugger structs
struct MEMPAGE
{
    MEMORY_BASIC_INFORMATION mbi;
    char mod[MAX_MODULE_SIZE];
};

struct MEMMAP
{
    int count;
    MEMPAGE* page;
};

struct BRIDGEBP
{
    BPXTYPE type;
    duint addr;
    bool enabled;
    bool singleshoot;
    bool active;
    char name[MAX_BREAKPOINT_SIZE];
    char mod[MAX_MODULE_SIZE];
    unsigned short slot;
};

struct BPMAP
{
    int count;
    BRIDGEBP* bp;
};

struct ADDRINFO
{
    char module[MAX_MODULE_SIZE]; //module the address is in
    char label[MAX_LABEL_SIZE];
    char comment[MAX_COMMENT_SIZE];
    bool isbookmark;
    int flags; //ADDRINFOFLAGS
};

struct FLAGS
{
    bool c;
    bool p;
    bool a;
    bool z;
    bool s;
    bool t;
    bool i;
    bool d;
    bool o;
};

struct REGDUMP
{
    duint cax;
    duint ccx;
    duint cdx;
    duint cbx;
    duint csp;
    duint cbp;
    duint csi;
    duint cdi;
#ifdef _WIN64
    duint r8;
    duint r9;
    duint r10;
    duint r11;
    duint r12;
    duint r13;
    duint r14;
    duint r15;
#endif //_WIN64
    duint cip;
    unsigned int eflags;
    FLAGS flags;
    unsigned short gs;
    unsigned short fs;
    unsigned short es;
    unsigned short ds;
    unsigned short cs;
    unsigned short ss;
    duint dr0;
    duint dr1;
    duint dr2;
    duint dr3;
    duint dr6;
    duint dr7;
};

//Debugger functions
BRIDGE_IMPEXP void DbgMemRead(duint va, unsigned char* dest, duint size);
BRIDGE_IMPEXP duint DbgMemGetPageSize(duint base);
BRIDGE_IMPEXP duint DbgMemFindBaseAddr(duint addr, duint* size);
BRIDGE_IMPEXP bool DbgCmdExec(const char* cmd);
BRIDGE_IMPEXP bool DbgCmdExecDirect(const char* cmd);
BRIDGE_IMPEXP bool DbgMemMap(MEMMAP* memmap);
BRIDGE_IMPEXP bool DbgIsValidExpression(const char* expression);
BRIDGE_IMPEXP bool DbgIsDebugging();
BRIDGE_IMPEXP bool DbgIsJumpGoingToExecute(duint addr);
BRIDGE_IMPEXP bool DbgGetLabelAt(duint addr, SEGMENTREG segment, char* text);
BRIDGE_IMPEXP bool DbgSetLabelAt(duint addr, const char* text);
BRIDGE_IMPEXP bool DbgGetCommentAt(duint addr, char* text);
BRIDGE_IMPEXP bool DbgSetCommentAt(duint addr, const char* text);
BRIDGE_IMPEXP bool DbgGetBookmarkAt(duint addr);
BRIDGE_IMPEXP bool DbgSetBookmarkAt(duint addr, bool isbookmark);
BRIDGE_IMPEXP bool DbgGetModuleAt(duint addr, char* text);
BRIDGE_IMPEXP BPXTYPE DbgGetBpxTypeAt(duint addr);
BRIDGE_IMPEXP duint DbgValFromString(const char* string);
BRIDGE_IMPEXP bool DbgGetRegDump(REGDUMP* regdump);
BRIDGE_IMPEXP bool DbgValToString(const char* string, duint value);
BRIDGE_IMPEXP bool DbgMemIsValidReadPtr(duint addr);
BRIDGE_IMPEXP BPXTYPE DbgGetBpxTypeAt(duint addr);
BRIDGE_IMPEXP int DbgGetBpList(BPXTYPE type, BPMAP* list);

//GUI functions
BRIDGE_IMPEXP void GuiDisasmAt(duint addr, duint cip);
BRIDGE_IMPEXP void GuiSetDebugState(DBGSTATE state);
BRIDGE_IMPEXP void GuiAddLogMessage(const char* msg);
BRIDGE_IMPEXP void GuiLogClear();
BRIDGE_IMPEXP void GuiUpdateAllViews();
BRIDGE_IMPEXP void GuiUpdateRegisterView();
BRIDGE_IMPEXP void GuiUpdateDisassemblyView();

#ifdef __cplusplus
}
#endif

#endif // _BRIDGEMAIN_H_
