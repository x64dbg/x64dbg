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

#ifndef DLL_IMPEXP
#ifdef BUILD_BRIDGE
#define DLL_IMPEXP __declspec(dllexport)
#else
#define DLL_IMPEXP __declspec(dllimport)
#endif //BUILD_BRIDGE
#endif //DLL_IMPEXP

#ifdef __cplusplus
extern "C"
{
#endif

//Bridge functions
DLL_IMPEXP const char* BridgeInit();
DLL_IMPEXP const char* BridgeStart();
DLL_IMPEXP void* BridgeAlloc(size_t size);
DLL_IMPEXP void BridgeFree(void* ptr);

//Debugger defines
#define MAX_LABEL_SIZE 256
#define MAX_COMMENT_SIZE 256

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
    module=1,
    label=2,
    comment=4
};

enum BPXTYPE
{
    bpnone,
    bpnormal,
    bphardware,
    bpmemory
};

//Debugger structs
struct MEMPAGE
{
    MEMORY_BASIC_INFORMATION mbi;
    char mod[32];
};

struct MEMMAP
{
    int count;
    MEMPAGE* page;
};

struct ADDRINFO
{
    char module[32]; //module the address is in
    char label[MAX_LABEL_SIZE];
    char comment[MAX_COMMENT_SIZE];
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
DLL_IMPEXP void DbgMemRead(duint va, unsigned char* dest, duint size);
DLL_IMPEXP duint DbgMemGetPageSize(duint base);
DLL_IMPEXP duint DbgMemFindBaseAddr(duint addr, duint* size);
DLL_IMPEXP bool DbgCmdExec(const char* cmd);
DLL_IMPEXP bool DbgMemMap(MEMMAP* memmap);
DLL_IMPEXP bool DbgIsValidExpression(const char* expression);
DLL_IMPEXP bool DbgIsDebugging();
DLL_IMPEXP bool DbgIsJumpGoingToExecute(duint addr);
DLL_IMPEXP bool DbgGetLabelAt(duint addr, SEGMENTREG segment, char* text);
DLL_IMPEXP bool DbgSetLabelAt(duint addr, const char* text);
DLL_IMPEXP bool DbgGetCommentAt(duint addr, char* text);
DLL_IMPEXP bool DbgSetCommentAt(duint addr, const char* text);
DLL_IMPEXP bool DbgGetModuleAt(duint addr, char* text);
DLL_IMPEXP BPXTYPE DbgGetBpxTypeAt(duint addr);
DLL_IMPEXP duint DbgValFromString(const char* string);
DLL_IMPEXP bool DbgGetRegDump(REGDUMP* regdump);
DLL_IMPEXP bool DbgValToString(const char* string, duint value);

//GUI functions
DLL_IMPEXP void GuiDisasmAt(duint addr, duint cip);
DLL_IMPEXP void GuiSetDebugState(DBGSTATE state);
DLL_IMPEXP void GuiAddLogMessage(const char* msg);
DLL_IMPEXP void GuiLogClear();
DLL_IMPEXP void GuiUpdateRegisterView();

#ifdef __cplusplus
}
#endif

#endif // _BRIDGEMAIN_H_
