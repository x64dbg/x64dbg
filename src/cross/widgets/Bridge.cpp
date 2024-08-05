#include <cstdio>

#include <QGuiApplication>
#include <QClipboard>

#include "Bridge.h"

#include "Types.h"
#include "Bridge.h"
#include <zydis_wrapper.h>

struct InvalidMemoryProvider : MemoryProvider
{
    bool read(duint addr, void* dest, duint size) override
    {
        return false;
    }

    bool getRange(duint addr, duint & base, duint & size) override
    {
        return false;
    }

    bool isCodePtr(duint addr) override
    {
        return false;
    }

    bool isValidPtr(duint addr) override
    {
        return false;
    }
} gInvalidMemoryProvider;

static MemoryProvider* gMemory = &gInvalidMemoryProvider;

void DbgSetMemoryProvider(MemoryProvider* provider)
{
    gMemory = provider ? provider : &gInvalidMemoryProvider;
}

// BRIDGE

bool BridgeSettingGet(const char* section, const char* key, char* value)
{
    return false;
}

bool BridgeSettingSet(const char* section, const char* key, const char* value)
{
    return false;
}

bool BridgeSettingGetUint(const char* section, const char* key, duint* value)
{
    return false;
}

bool BridgeSettingSetUint(const char* section, const char* key, duint value)
{
    return false;
}

const wchar_t* BridgeUserDirectory()
{
    return L".";
}

void* BridgeAlloc(size_t size)
{
    return malloc(size);
}

void BridgeFree(void* ptr)
{
    free(ptr);
}

// DBG

bool DbgIsDebugging()
{
    return true;
}

DBGFUNCTIONS* DbgFunctions()
{
    static DBGFUNCTIONS* cache = []
    {
        static DBGFUNCTIONS f;
        f.GetTraceRecordHitCount = [](duint addr) -> duint
        {
            return 0;
        };
        f.ModBaseFromAddr = [](duint addr) -> duint
        {
            return 0;
        };
        f.ModNameFromAddr = [](duint addr, char* name, bool extension)
        {
            return false;
        };
        f.StringFormatInline = [](char* dest, duint size, const char* format)
        {
            return false;
        };
        f.MemIsCodePage = [](duint addr, bool refresh)
        {
            return gMemory->isCodePtr(addr);
        };
        f.GetMnemonicBrief = [](const char* mnem, size_t resultSize, char* result)
        {
            *result = '\0';
        };
        f.ModRelocationAtAddr = [](duint addr, DBGRELOCATIONINFO * relocation)
        {
            return false;
        };
        f.PatchGetEx = [](duint addr, DBGPATCHINFO * info)
        {
            return false;
        };
        f.ValFromString = [](const char* expr, duint * value)
        {
            bool success = false;
            *value = DbgEval(expr, &success);
            return success;
        };
        f.ModGetParty = [](duint addr)
        {
            return 0;
        };
        f.PatchInRange = [](duint start, duint end)
        {
            return false;
        };
        f.MemPatch = [](duint start, const unsigned char* data, duint size)
        {
            return false;
        };
        return &f;
    }();
    return cache;
}

bool DbgGetLabelAt(duint addr, SEGTYPE seg, char* label)
{
    return false;
}

bool DbgGetModuleAt(duint addr, char* module)
{
    return false;
}

bool DbgGetCommentAt(duint addr, char* comment)
{
    return false;
}

bool DbgGetBookmarkAt(duint addr)
{
    return false;
}

BPXTYPE DbgGetBpxTypeAt(duint addr)
{
    return bp_none;
}

bool DbgMemIsValidReadPtr(duint addr)
{
    return gMemory->isValidPtr(addr);
}

bool DbgGetStringAt(duint addr, char* str)
{
    return false;
}

bool DbgEval(const char* expr, bool* success)
{
    return false;
}

duint DbgValFromString(const char* expr)
{
    duint result = 0;
    if(!DbgEval(expr))
        result = 0;
    return result;
}

bool DbgCmdExec(const char* cmd)
{
    printf("DbgCmdExec(\"%s\")\n", cmd);
    return false;
}

bool DbgCmdExecDirect(const char* cmd)
{
    printf("DbgCmdExecDirect(\"%s\")\n", cmd);
    return false;
}

bool DbgCmdExec(const QString & cmd)
{
    return DbgCmdExec(cmd.toUtf8().constData());
}

bool DbgCmdExecDirect(const QString & cmd)
{
    return DbgCmdExecDirect(cmd.toUtf8().constData());
}

duint DbgMemFindBaseAddr(duint addr, duint* size)
{
    duint rangeBase = 0;
    duint rangeSize = 0;
    if(!gMemory->getRange(addr, rangeBase, rangeSize))
        return 0;

    if(size != nullptr)
        *size = rangeSize;

    return rangeBase;
}

bool DbgMemRead(duint addr, void* dest, size_t size)
{
    return gMemory->read(addr, dest, size);
}

FUNCTYPE DbgGetFunctionTypeAt(duint addr)
{
    return FUNC_NONE;
}

XREFTYPE DbgGetXrefTypeAt(duint addr)
{
    return XREF_NONE;
}

ARGTYPE DbgGetArgTypeAt(duint addr)
{
    return ARG_NONE;
}

LOOPTYPE DbgGetLoopTypeAt(duint addr, int depth)
{
    return LOOP_NONE;
}

duint DbgGetBranchDestination(duint addr)
{
    uint8_t data[MAX_DISASM_BUFFER];
    if(!DbgMemRead(addr, data, sizeof(data)))
        return 0;
    Zydis zydis(true); // TODO: architecture
    if(!zydis.Disassemble(addr, data))
        return 0;
    return zydis.BranchDestination();
}

bool DbgIsJumpGoingToExecute(duint addr)
{
    return false; // TODO
}

bool DbgXrefGet(duint addr, XREF_INFO* info)
{
    return false;
}

void DbgReleaseEncodeTypeBuffer(void* buffer)
{
    BridgeFree(buffer);
}

void* DbgGetEncodeTypeBuffer(duint addr, duint* size)
{
    return nullptr;
}

bool DbgSetEncodeType(duint addr, duint size, ENCODETYPE type)
{
    return false;
}

void DbgDelEncodeTypeRange(duint start, duint end)
{
}

void DbgDelEncodeTypeSegment(duint start)
{
}

// GUI

void GuiExecuteOnGuiThreadEx(GuiCallback callback, void* data)
{
    // TODO: force schedule on the GUI thread
    callback(data);
}

void GuiAddLogMessage(const char* msg)
{
    Bridge::getBridge()->addMsgToLog(msg);
}

void GuiUpdateAllViews()
{
    Bridge::getBridge()->repaintTableView();
}

void GuiUpdatePatches()
{
}

Bridge* Bridge::getBridge()
{
    static Bridge i;
    return &i;
}

void Bridge::CopyToClipboard(const QString & str)
{
    QGuiApplication::clipboard()->setText(str);
}

void Bridge::addMsgToLog(const QByteArray & bytes)
{
    printf("addMsgToLog: %s\n", bytes.data());
}
