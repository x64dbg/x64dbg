
#include <atomic>
#include <QGuiApplication>
#include <QClipboard>
#include <QThread>
#include <QCoreApplication>

#include "Bridge.h"
#include "Types.h"
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

static std::atomic<MemoryProvider*> gMemory{&gInvalidMemoryProvider};

void DbgSetMemoryProvider(MemoryProvider* provider)
{
    gMemory.store(provider ? provider : &gInvalidMemoryProvider);
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
    return gMemory.load() != &gInvalidMemoryProvider;
}

DBGFUNCTIONS* DbgFunctions()
{
    static DBGFUNCTIONS f = []
    {
        DBGFUNCTIONS f{};
        f.GetTraceRecordHitCount = [](duint addr) -> duint
        {
            return 0;
        };
        f.GetTraceRecordByteType = [](duint addr)
        {
            return TRACERECORDBYTETYPE::TraceRecordByteTypeUnknown;
        };
        f.ModBaseFromAddr = [](duint addr) -> duint
        {
            duint base = 0;
            if(!gMemory.load()->modBaseFromAddr(addr, base))
                return 0;
            return base;
        };
        f.ModNameFromAddr = [](duint addr, char* name, bool extension)
        {
            return gMemory.load()->modNameFromAddr(addr, name, MAX_MODULE_SIZE, extension);
        };
        f.StringFormatInline = [](const char* format, size_t size, char* dest)
        {
            return false;
        };
        f.MemIsCodePage = [](duint addr, bool refresh)
        {
            return gMemory.load()->isCodePtr(addr);
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
            return gMemory.load()->write(start, data, size);
        };
        return f;
    }();
    return &f;
}

// TODO: resolve via per-module .dynsym lookup.
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

// TODO: persist address colors in the cross debugger database.
bool DbgGetAddressColorAt(duint addr, unsigned int* color)
{
    return false;
}

bool DbgSetAddressColorRange(duint start, duint end, unsigned int color)
{
    return false;
}

void DbgDelAddressColorRange(duint start, duint end)
{
}

bool DbgGetBookmarkAt(duint addr)
{
    return false;
}

static std::atomic<BreakpointQueryFunc> gBreakpointQuery{nullptr};

void DbgSetBreakpointQuery(BreakpointQueryFunc func)
{
    gBreakpointQuery.store(func);
}

BPXTYPE DbgGetBpxTypeAt(duint addr)
{
    auto query = gBreakpointQuery.load();
    if(query)
        return query(addr);
    return bp_none;
}

bool DbgMemIsValidReadPtr(duint addr)
{
    return gMemory.load()->isValidPtr(addr);
}

// TODO: detect printable ASCII / UTF-16LE string at addr.
bool DbgGetStringAt(duint addr, char* str)
{
    return false;
}

duint DbgEval(const char* expr, bool* success)
{
    // TODO: replace this hex-only stub with a real cross expression evaluator (breaks rsp+8, symbols, mod.main(), etc.).
    if(success)
        *success = false;
    if(!expr)
        return 0;

    QString str = QString::fromUtf8(expr).trimmed();
    if(str.isEmpty())
        return 0;

    bool negative = false;
    if(str.startsWith('-'))
    {
        negative = true;
        str = str.mid(1).trimmed();
    }
    if(str.startsWith("0x", Qt::CaseInsensitive))
        str = str.mid(2);

    bool ok = false;
    const duint value = str.toULongLong(&ok, 16);
    if(!ok)
        return 0;

    if(success)
        *success = true;
    return negative ? static_cast<duint>(0) - value : value;
}

duint DbgValFromString(const char* expr)
{
    return DbgEval(expr, nullptr);
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
    if(!gMemory.load()->getRange(addr, rangeBase, rangeSize))
        return 0;

    if(size != nullptr)
        *size = rangeSize;

    return rangeBase;
}

bool DbgMemRead(duint addr, void* dest, size_t size)
{
    return gMemory.load()->read(addr, dest, size);
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
    Zydis zydis(sizeof(duint) == 8); // TODO: architecture plumbed per-caller
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
    if(QThread::currentThread() == QCoreApplication::instance()->thread())
    {
        callback(data);
        return;
    }
    QMetaObject::invokeMethod(QCoreApplication::instance(), [callback, data]()
    {
        callback(data);
    }, Qt::QueuedConnection);
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
