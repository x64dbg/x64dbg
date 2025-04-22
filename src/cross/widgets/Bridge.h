#pragma once

#include <QObject>
#include <QString>
#include <QDebug>

#include "Types.h"

#define MAX_SETTING_SIZE 4096

bool BridgeSettingGet(const char* section, const char* key, char* value);
bool BridgeSettingSet(const char* section, const char* key, const char* value);
bool BridgeSettingGetUint(const char* section, const char* key, duint* value);
bool BridgeSettingSetUint(const char* section, const char* key, duint value);
const wchar_t* BridgeUserDirectory();
void* BridgeAlloc(size_t size);
void BridgeFree(void* ptr);

#define MAX_LABEL_SIZE 256
#define MAX_MODULE_SIZE 256
#define MAX_COMMENT_SIZE 256
#define MAX_STRING_SIZE 2048

enum SEGTYPE
{
    SEG_DEFAULT,
    SEG_ES,
    SEG_DS,
    SEG_FS,
    SEG_GS,
    SEG_CS,
    SEG_SS,
};

enum BPXTYPE
{
    bp_none,
    bp_normal,
    bp_hardware,
    bp_memory,
};

typedef enum
{
    FUNC_NONE,
    FUNC_BEGIN,
    FUNC_MIDDLE,
    FUNC_END,
    FUNC_SINGLE
} FUNCTYPE;

typedef enum
{
    LOOP_NONE,
    LOOP_BEGIN,
    LOOP_MIDDLE,
    LOOP_ENTRY,
    LOOP_END,
    LOOP_SINGLE
} LOOPTYPE;

typedef enum
{
    ARG_NONE,
    ARG_BEGIN,
    ARG_MIDDLE,
    ARG_END,
    ARG_SINGLE
} ARGTYPE;

typedef struct
{
    uint32_t rva;
    uint8_t type;
    uint16_t size;
} DBGRELOCATIONINFO;

typedef struct
{
    char mod[MAX_MODULE_SIZE];
    duint addr;
    unsigned char oldbyte;
    unsigned char newbyte;
} DBGPATCHINFO;

struct DBGFUNCTIONS
{
    duint(*GetTraceRecordHitCount)(duint addr);
    duint(*ModBaseFromAddr)(duint addr);
    bool (*ModNameFromAddr)(duint base, char* name, bool extension);
    bool (*StringFormatInline)(char* dest, duint size, const char* format);
    bool (*MemIsCodePage)(duint addr, bool refresh);
    void (*GetMnemonicBrief)(const char* mnem, size_t resultSize, char* result);
    bool (*ModRelocationAtAddr)(duint addr, DBGRELOCATIONINFO* relocation);
    bool (*PatchGetEx)(duint addr, DBGPATCHINFO* info);
    bool (*ValFromString)(const char* expr, duint* value);
    int (*ModGetParty)(duint addr);
    bool (*PatchInRange)(duint start, duint end);
    bool (*MemPatch)(duint start, const unsigned char* data, duint size);
};

struct MemoryProvider
{
    virtual ~MemoryProvider() = default;
    virtual bool read(duint addr, void* dest, duint size) = 0;
    virtual bool getRange(duint addr, duint & base, duint & size) = 0;
    virtual bool isCodePtr(duint addr) = 0;
    virtual bool isValidPtr(duint addr) = 0;
};

void DbgSetMemoryProvider(MemoryProvider* provider);

bool DbgIsDebugging();
DBGFUNCTIONS* DbgFunctions();
bool DbgGetLabelAt(duint addr, SEGTYPE seg, char* label);
bool DbgGetModuleAt(duint addr, char* module);
bool DbgGetCommentAt(duint addr, char* comment);
bool DbgGetBookmarkAt(duint addr);
BPXTYPE DbgGetBpxTypeAt(duint addr);
bool DbgMemIsValidReadPtr(duint addr);
bool DbgGetStringAt(duint addr, char* str);
bool DbgEval(const char* expr, bool* success = nullptr);
duint DbgValFromString(const char* expr);
bool DbgCmdExec(const char* cmd);
bool DbgCmdExecDirect(const char* cmd);
duint DbgMemFindBaseAddr(duint addr, duint* size);
bool DbgMemRead(duint addr, void* dest, size_t size);
FUNCTYPE DbgGetFunctionTypeAt(duint addr);
XREFTYPE DbgGetXrefTypeAt(duint addr);
ARGTYPE DbgGetArgTypeAt(duint addr);
LOOPTYPE DbgGetLoopTypeAt(duint addr, int depth);
duint DbgGetBranchDestination(duint addr);
bool DbgIsJumpGoingToExecute(duint addr);
bool DbgXrefGet(duint addr, XREF_INFO* info);
void DbgReleaseEncodeTypeBuffer(void* buffer);
void* DbgGetEncodeTypeBuffer(duint addr, duint* size);
bool DbgSetEncodeType(duint addr, duint size, ENCODETYPE type);
void DbgDelEncodeTypeRange(duint start, duint end);
void DbgDelEncodeTypeSegment(duint start);

struct TYPEDESCRIPTOR;

typedef bool (*TYPETOSTRING)(const TYPEDESCRIPTOR* type, char* dest, size_t* destCount); //don't change destCount for final failure

#define TYPEDESCRIPTOR_MAGIC 0x1337

struct TYPEDESCRIPTOR
{
    bool expanded; //is the type node expanded?
    bool reverse; //big endian?
    uint16_t magic; // compatiblity
    const char* name; //type name (int b)
    duint addr; //virtual address
    duint offset; //offset to addr for the actual location in bytes
    int id; //type id
    int sizeBits; //sizeof(type) in bits
    TYPETOSTRING callback; //convert to string
    void* userdata; //user data
    duint bitOffset; // bit offset from first bitfield
};

using GuiCallback = void(*)(void*);

void GuiExecuteOnGuiThreadEx(GuiCallback callback, void* data);
void GuiAddLogMessage(const char* msg);
void GuiUpdateAllViews();
void GuiUpdatePatches();

// QString helpers
bool DbgCmdExec(const QString & cmd);
bool DbgCmdExecDirect(const QString & cmd);

class Bridge : public QObject
{
    Q_OBJECT
    Bridge() = default;

signals:
    void close();
    void repaintTableView();
    void updateDump();
    void updateDisassembly();
    void dbgStateChanged(DBGSTATE state);

public:
    static Bridge* getBridge();
    static void CopyToClipboard(const QString & str);

    void addMsgToLog(const QByteArray & bytes);

    duint mLastCip = 0;
    bool mIsRunning = true;
};
